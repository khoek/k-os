#include "init/initcall.h"
#include "lib/string.h"
#include "common/compiler.h"
#include "common/math.h"
#include "bug/panic.h"
#include "arch/bios.h"
#include "arch/acpi.h"
#include "arch/apic.h"
#include "arch/mp.h"
#include "arch/pic.h"
#include "arch/pit.h"
#include "arch/hpet.h"
#include "bug/panic.h"
#include "mm/mm.h"
#include "log/log.h"

#define ACPI_SIG_RSDP "RSD PTR "
#define ACPI_SIG_RSDT "RSDT"
#define ACPI_SIG_MADT "APIC"
#define ACPI_SIG_HPET "HPET"

typedef struct acpi_rsdp {
    uint8_t sig[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t rev;
    phys_addr_t rsdt;
} PACKED acpi_rsdp_t;

static inline bool acpi_checksum(void *ptr, uint32_t bytes) {
    uint8_t sum = 0;
    for(uint32_t i = 0; i < bytes; i++) {
        sum += *(((uint8_t *) ptr) + i);
    }

    return !sum;
}

static inline bool acpi_sig_match(void *sig, acpi_sdt_t *sdt) {
    return !memcmp(sig, sdt->sig, sizeof(sdt->sig));
}

static inline bool acpi_valid_rsdp(acpi_rsdp_t *rsdp) {
    if(memcmp(ACPI_SIG_RSDP, rsdp->sig, sizeof(rsdp->sig))) return false;
    return acpi_checksum(rsdp, sizeof(acpi_rsdp_t));
}

static inline bool acpi_valid_sdt(acpi_sdt_t *sdt) {
    return acpi_checksum(sdt, sdt->len);
}
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

void madt_parse(acpi_sdt_t *madt) {
    acpi_madt_t *madt_data = (void *) madt->data;

    apic_init(madt_data->lapic_base);

    uint8_t bsp_id = apic_get_id();

    uint32_t entry_ap_len = ((uint32_t) &entry_ap_end) - ((uint32_t) &entry_ap_start);
    uint32_t entry_ap_num_pages = DIV_UP(entry_ap_len, PAGE_SIZE);
    phys_addr_t entry_ap_dst_phys = ENTRY_AP_BASE;
    void *entry_ap_dst_virt = map_pages(entry_ap_dst_phys, entry_ap_num_pages);
    void *entry_ap_src_virt = map_pages((uint32_t) &entry_ap_start, entry_ap_num_pages);

    memcpy(entry_ap_dst_virt, entry_ap_src_virt, entry_ap_len);
    //TODO unmap the pages

    //apic_init invokes sti()
    irqdisable();
    outb(0x70, 0xF);
    outb(0x71, 0xA);
    irqenable();

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

static INITCALL acpi_init() {
    acpi_rsdp_t *rsdp;
    acpi_sdt_t *rsdt;

    kprintf("acpi - parsing tables");

    uint8_t *search = map_pages(VRAM_START, VRAM_PAGES);

    uint32_t inc = 0;
    while(inc < (VRAM_END - VRAM_START)) {
        rsdp = (acpi_rsdp_t *) (search + inc);
        if(acpi_valid_rsdp(rsdp)) {
            break;
        }

        rsdp = NULL;
        inc += 16;
    }

    kprintf("acpi - RSDP @ 0x%X", virt_to_phys(rsdp));

    acpi_sdt_t *madt = NULL;
    acpi_sdt_t *hpet = NULL;

    if(rsdp) {
        rsdt = map_page(rsdp->rsdt);

        if(acpi_sig_match(ACPI_SIG_RSDT, rsdt) && acpi_valid_sdt(rsdt)) {
            phys_addr_t *sdts = (void *) rsdt->data;
            for(uint32_t i = 0; i < (rsdt->len - sizeof(acpi_sdt_t)) / sizeof(void *); i++) {
                //FIXME unmap these pages
                acpi_sdt_t *real = map_page(sdts[i]);

                /* Does the SDT header overflow onto the next page? */
                if((((uint32_t) real) / PAGE_SIZE) < ((((uint32_t) real) + sizeof(acpi_sdt_t)) / PAGE_SIZE)) {
                    map_page(sdts[i] + PAGE_SIZE);
                }

                if(real->len < sizeof(acpi_sdt_t)) {
                    continue;
                }

                /* Does the SDT body overflow onto more pages? */
                for(uint32_t i = 0; i < ((((uint32_t) real) + real->len) / PAGE_SIZE) - ((((uint32_t) real) + sizeof(acpi_sdt_t)) / PAGE_SIZE); i++) {
                    map_page(sdts[i] + (PAGE_SIZE * (i + 1)));
                }

                kprintf("acpi - found table %c%c%c%c @ %X",
                    real->sig[0], real->sig[1],
                    real->sig[2], real->sig[3], virt_to_phys(real));

                if(acpi_valid_sdt(real)) {
                    if(acpi_sig_match(ACPI_SIG_MADT, real)) {
                        madt = real;
                    } else if(acpi_sig_match(ACPI_SIG_HPET, real)) {
                        hpet = real;
                    }
                }
            }

            kprintf("acpi - MADT @ 0x%X", virt_to_phys(madt));
            kprintf("acpi - HPET @ 0x%X", virt_to_phys(hpet));

            if(madt) {
                madt_parse(madt);
                if(hpet) hpet_init(hpet);
            }
        } else {
            rsdt = NULL;

            kprintf("acpi - RSDT invalid!");
        }
    }

    if(!madt) {
        kprintf("acpi - pic fallback");
        pic_init();
    }

    if(!madt || !hpet) {
        kprintf("acpi - pit fallback");
        pit_init();
    }

    kprintf("acpi - initialized successfully");

    return 0;
}

postarch_initcall(acpi_init);
