#include "init/initcall.h"
#include "lib/string.h"
#include "common/compiler.h"
#include "bug/panic.h"
#include "arch/bios.h"
#include "arch/acpi.h"
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

                if(acpi_valid_sdt(real)) {
                    if(acpi_sig_match(ACPI_SIG_MADT, real)) {
                        madt = real;
                    } else if(acpi_sig_match(ACPI_SIG_HPET, real)) {
                        hpet = real;
                    }
                }
            }

            if(madt) {
                kprintf("acpi - MADT detected at 0x%X", virt_to_phys(madt));

                madt_parse(madt);
                if(hpet) hpet_init(hpet);
            }
        } else {
            rsdt = NULL;

            kprintf("acpi - invalid rsdt");
        }
    } else {
        kprintf("acpi - no rsdp detected");
    }

    if(!madt) pic_init();

    if(!hpet) {
        pit_init();
    }

    kprintf("acpi - initialized successfully");

    return 0;
}

postarch_initcall(acpi_init);
