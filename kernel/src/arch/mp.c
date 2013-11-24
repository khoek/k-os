#include "lib/int.h"
#include "init/initcall.h"
#include "common/list.h"
#include "common/mmio.h"
#include "common/compiler.h"
#include "arch/acpi.h"
#include "arch/mp.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "sched/sched.h"
#include "sched/proc.h"
#include "video/log.h"

extern uint32_t entry_ap;

static uint32_t count;
static void *lapic_base;
static volatile bool waiting = false;

static volatile uint32_t next_ap_acpi_id;
volatile void *next_ap_stack;

#define PROCESSOR_FOR_EACH(ptr) LIST_FOR_EACH_ENTRY(ptr, &procs, list)

#define LAPIC_REG_ID       0x020
#define LAPIC_REG_ICR_LOW  0x300
#define LAPIC_REG_ICR_HIGH 0x310

#define LAPIC_CMD_TYPE_INIT    (0x5)
#define LAPIC_CMD_TYPE_STARTUP (0x6)

#define LAPIC_CMD_FLAG_PENDING (1 << 12)
#define LAPIC_CMD_FLAG_ASSERT  (1 << 14)

static inline uint32_t apic_get_id() {
    return readl(lapic_base, LAPIC_REG_ID) >> 24;
}

static void apic_issue_command(void *lapic_base, uint8_t target_id, uint16_t type, uint32_t flags, uint8_t vector) {
    writel(lapic_base, LAPIC_REG_ICR_HIGH, target_id << 24);
    writel(lapic_base, LAPIC_REG_ICR_LOW , (type << 8) | flags | vector);

    while(readl(lapic_base, LAPIC_REG_ICR_LOW) & LAPIC_CMD_FLAG_PENDING);
}

void __noreturn mp_ap_start() {
    waiting = false;

    static volatile uint32_t num = 1;
    processor_t *me = register_proc(num++);
    me->arch.acpi_id = next_ap_acpi_id;
    me->arch.apic_id = apic_get_id();

    sched_loop();
}

void __init mp_init(acpi_sdt_t *madt) {
    if(!madt) {
        logf("mp - UP system detected");
        count = 1;

        return;
    }

    logf("mp - MP-compliant system detected");

    acpi_madt_t *madt_data = (void *) madt->data;
    lapic_base = map_page(madt_data->lapic_base);
    uint8_t bsp_id = apic_get_id();

    uint8_t *upto = madt_data->records;
    while(upto < madt_data->records + (madt->len - (sizeof(acpi_sdt_t) + sizeof(acpi_madt_t)))) {
        uint8_t type = upto[0];
        uint8_t size = upto[1];

        if(upto + size >= madt->data + madt->len - sizeof(acpi_sdt_t)) break;

        switch(type) {
            case MADT_ENTRY_TYPE_LAPIC: {
                uint32_t flags = *((uint32_t *) &upto[4]);
                if(flags & MADT_ENTRY_LAPIC_FLAG_ENABLED) {
                    uint32_t id = upto[3];

                    if(id != bsp_id) {
                        next_ap_acpi_id = upto[2];
                        page_t *page = alloc_pages(STACK_NUM_PAGES, 0);
                        next_ap_stack = (void *) ((uint32_t) page_to_virt(page) + PAGE_SIZE);

                        apic_issue_command(lapic_base, id, LAPIC_CMD_TYPE_INIT, LAPIC_CMD_FLAG_ASSERT, 0x00);
                        sleep(1); //FIXME 10 milliseconds
                        apic_issue_command(lapic_base, id, LAPIC_CMD_TYPE_STARTUP, LAPIC_CMD_FLAG_ASSERT, (((uint32_t) &entry_ap) >> 12) & 0xFF);

                        waiting = true;
                        while(waiting);
                    }
                }

                break;
            }
        };

        upto += size;
    }
}
