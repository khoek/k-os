#include "common/compiler.h"
#include "init/initcall.h"
#include "arch/bios.h"
#include "arch/acpi.h"
#include "arch/apic.h"
#include "arch/mp.h"
#include "mm/mm.h"
#include "video/log.h"

#define ENTRY_TYPE_PROC 0

#define PROC_FLAG_ENABLED (1 << 0)

typedef struct madt_entry_header {
    uint8_t type;
    uint8_t len;
} PACKED madt_entry_header_t;

typedef struct proc_entry {
    uint8_t proc_id;
    uint8_t lapic_id;
    uint32_t flags;
} PACKED proc_entry_t;

static uint32_t count = 1;

void madt_parse(acpi_sdt_t *madt) {
    acpi_madt_t *madt_data = (void *) madt->data;

    apic_init(madt_data->lapic_base);

    uint8_t bsp_id = apic_get_id();

    uint32_t entry_ap_len = ((uint32_t) &entry_ap_end) - ((uint32_t) &entry_ap_start);
    uint32_t entry_ap_num_pages = DIV_UP(entry_ap_len, PAGE_SIZE);
    void *entry_ap_dst_phys = (void *) ENTRY_AP_BASE;
    void *entry_ap_dst_virt = map_pages(entry_ap_dst_phys, entry_ap_num_pages);
    void *entry_ap_src_virt = map_pages(&entry_ap_start, entry_ap_num_pages);

    memcpy(entry_ap_dst_virt, entry_ap_src_virt, entry_ap_len);
    //TODO unmap the pages

    cli();
    outb(0x70, 0xF);
    outb(0x71, 0xA);
    sti();

    bda_putl(BDA_RESET_VEC, (uint32_t) entry_ap_dst_phys);

    uint32_t off = 0;
    while(off < madt->len - (sizeof(acpi_sdt_t) + sizeof(acpi_madt_t))) {
        madt_entry_header_t *ehdr = ((void *) madt->data) + sizeof(acpi_madt_t) + off;
        off += ehdr->len;
        switch(ehdr->type) {
            case ENTRY_TYPE_PROC: {
                proc_entry_t *proc = ((void *) ehdr) + sizeof(madt_entry_header_t);
                if(proc->flags & PROC_FLAG_ENABLED) {
                    if(proc->lapic_id != bsp_id) {
                        count++;

                        next_ap_acpi_id = proc->proc_id;
                        page_t *page = alloc_pages(STACK_NUM_PAGES, 0);
                        next_ap_stack = (void *) ((uint32_t) page_to_virt(page) + PAGE_SIZE);

                        ap_starting = true;

                        kprintf("acpi - commanding processor 0x%X:0x%X to start", proc->proc_id, proc->lapic_id);

                        apic_issue_command(proc->lapic_id, APIC_CMD_TYPE_INIT, APIC_CMD_FLAG_ASSERT, 0x00);
                        //TODO sleep for 10ms using the apic timer
                        apic_issue_command(proc->lapic_id, APIC_CMD_TYPE_STARTUP, APIC_CMD_FLAG_ASSERT, (((uint32_t) entry_ap_dst_phys) >> 12) & 0xFF);

                        while(ap_starting);
                    }
                }

                break;
            }
        };
    }

    mp_init();
}
