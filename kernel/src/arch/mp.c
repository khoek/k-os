#include "lib/int.h"
#include "init/initcall.h"
#include "common/list.h"
#include "common/compiler.h"
#include "arch/pic.h"
#include "arch/apic.h"
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

static uint32_t count = 1;
static volatile bool waiting = false;

static volatile uint32_t next_ap_acpi_id;
volatile void *next_ap_stack;

void __noreturn mp_ap_start() {
    waiting = false;

    static volatile uint32_t num = 1;
    processor_t *me = register_proc(num++);
    me->arch.acpi_id = next_ap_acpi_id;
    me->arch.apic_id = apic_get_id();

    apic_enable();

    sched_loop();
}

void __init mp_init(acpi_sdt_t *madt) {
    if(!madt) {
        panic("mp - no LAPIC detected!");
    }

    kprintf("mp - MP compliant system detected");

    acpi_madt_t *madt_data = (void *) madt->data;
    apic_init(madt_data->lapic_base);

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
                        count++;

                        next_ap_acpi_id = upto[2];
                        page_t *page = alloc_pages(STACK_NUM_PAGES, 0);
                        next_ap_stack = (void *) ((uint32_t) page_to_virt(page) + PAGE_SIZE);

                        waiting = true;

                        apic_issue_command(id, APIC_CMD_TYPE_INIT, APIC_CMD_FLAG_ASSERT, 0x00);
                        //TODO sleep for 10ms using the apic timer
                        apic_issue_command(id, APIC_CMD_TYPE_STARTUP, APIC_CMD_FLAG_ASSERT, (((uint32_t) &entry_ap) >> 12) & 0xFF);

                        while(waiting);
                    }
                }

                break;
            }
        };

        upto += size;
    }

    //TODO remove this and remap at apic level (requires ACPI parsing)
    pic_init();
}
