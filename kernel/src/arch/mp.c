#include "lib/int.h"
#include "init/initcall.h"
#include "common/list.h"
#include "common/mmio.h"
#include "common/compiler.h"
#include "arch/acpi.h"
#include "arch/mp.h"
#include "bug/panic.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "task/task.h"
#include "video/log.h"

struct processor {
    uint32_t num;
    uint8_t acpi_id;
    uint8_t apic_id;

    list_head_t list;
};

extern uint32_t entry_ap;

static uint32_t count;
static uint32_t up_processors = 1;
static void *lapic_base;
static volatile bool waiting = false;

processor_t *bsp;
static DEFINE_LIST(procs);

static void *next_ap_stack;

#define PROCESSOR_FOR_EACH(ptr) LIST_FOR_EACH_ENTRY(ptr, &procs, list)

#define LAPIC_REG_ID       0x020
#define LAPIC_REG_ICR_LOW  0x300
#define LAPIC_REG_ICR_HIGH 0x310

#define LAPIC_CMD_TYPE_INIT    (0x5)
#define LAPIC_CMD_TYPE_STARTUP (0x6)

#define LAPIC_CMD_FLAG_PENDING (1 << 12)
#define LAPIC_CMD_FLAG_ASSERT  (1 << 14)

static void apic_issue_command(void *lapic_base, uint8_t target_id, uint16_t type, uint32_t flags, uint8_t vector) {
    writel(lapic_base, LAPIC_REG_ICR_HIGH, target_id << 24);
    writel(lapic_base, LAPIC_REG_ICR_LOW , (type << 8) | flags | vector);

    while(readl(lapic_base, LAPIC_REG_ICR_LOW) & LAPIC_CMD_FLAG_PENDING);
}

static void __noreturn mp_ap_start() {
    uint8_t apic_id = readl(lapic_base, LAPIC_REG_ID) >> 24;

    processor_t *proc;
    PROCESSOR_FOR_EACH(proc) {
        if(proc->apic_id == apic_id) {
            //TODO do interrupts

            mp_run_cpu(proc);
        }
    }

    panicf("mp - unknown apic id %X", apic_id);
}

void mp_ap_init() {
    /*
        NOTE: there can be no local variables on the stack in this function; the swapping of stacks will cause a Page Fault
    */

    __asm__ volatile("mov %0, %%cr3" :: "a" (((uint32_t) page_directory) - VIRTUAL_BASE));
    __asm__ volatile("mov %0, %%esp" :: "a" (next_ap_stack));

    if(up_processors++ != count) {
        next_ap_stack = (void *) ((uint32_t) page_to_virt(alloc_pages(STACK_NUM_PAGES, 0)) + PAGE_SIZE);
    }

    mp_ap_start();
}

void mp_run_cpu(processor_t *proc) {
    logf("cpu #%u - ready", proc->num);
    waiting = false;
    while(1);

    task_run();

    panicf("cpu #%u -  returned from main loop!", proc->num);
}

static INITCALL mp_init() {
    if(!madt) {
        logf("mp - UP system detected");

        count = 1;

        bsp = kmalloc(sizeof(processor_t));
        bsp->num = 0;
        bsp->acpi_id = 0;
        bsp->apic_id = 0;

        list_add(&bsp->list, &procs);

        return 0;
    }

    logf("mp - MP-compliant system detected");

    acpi_madt_t *madt_data = (void *) madt->data;
    lapic_base = mm_map(madt_data->lapic_base);
    uint8_t bsp_id = readl(lapic_base, LAPIC_REG_ID) >> 24;

    uint8_t *upto = madt_data->records;
    while(upto < madt_data->records + (madt->len - (sizeof(acpi_sdt_t) + sizeof(acpi_madt_t)))) {
        uint8_t type = upto[0];
        uint8_t size = upto[1];

        if(upto + size >= madt->data + madt->len - sizeof(acpi_sdt_t)) break;

        switch(type) {
            case MADT_ENTRY_TYPE_LAPIC: {
                uint32_t flags = *((uint32_t *) &upto[4]);
                if(flags & MADT_ENTRY_LAPIC_FLAG_ENABLED) {
                    processor_t *proc = kmalloc(sizeof(processor_t));
                    proc->acpi_id = upto[2];
                    proc->apic_id = upto[3];

                    list_add_before(&proc->list, &procs);

                    if(proc->apic_id == bsp_id) {
                        proc->num = 0;
                        bsp = proc;
                    } else {
                        static uint32_t next_num = 1;
                        proc->num = next_num++;
                    }

                    count++;
                }

                break;
            }
        };

        upto += size;
    }

    if(count > 1) {
        next_ap_stack = (void *) ((uint32_t) page_to_virt(alloc_pages(STACK_NUM_PAGES, 0)) + PAGE_SIZE);
    }

    processor_t *proc;

    PROCESSOR_FOR_EACH(proc) {
        if(proc != bsp) {
            apic_issue_command(lapic_base, proc->apic_id, LAPIC_CMD_TYPE_INIT, LAPIC_CMD_FLAG_ASSERT, 0x00);
        }
    }

    sleep(1); //FIXME 10 milliseconds

    PROCESSOR_FOR_EACH(proc) {
        if(proc != bsp) {
            apic_issue_command(lapic_base, proc->apic_id, LAPIC_CMD_TYPE_STARTUP, LAPIC_CMD_FLAG_ASSERT, (((uint32_t) &entry_ap) >> 12) & 0xFF);

            waiting = true;
            while(waiting);
        }
    }

    return 0;
}

postarch_initcall(mp_init);
