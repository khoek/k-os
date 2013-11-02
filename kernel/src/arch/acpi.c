#include "init/initcall.h"
#include "lib/string.h"
#include "common/compiler.h"
#include "arch/bios.h"
#include "arch/acpi.h"
#include "mm/mm.h"
#include "video/log.h"

#define ACPI_SIG_RSDP "RSD PTR "
#define ACPI_SIG_RSDT "RSDT"
#define ACPI_SIG_MADT "APIC"

typedef struct acpi_rsdp {
    uint8_t sig[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t rev;
    uint8_t *rsdt;
} PACKED acpi_rsdp_t;

typedef struct acpi_madt {
    void *lapic_addr;
    uint32_t flags;
    uint8_t records[];
} PACKED acpi_madt_t;

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

static acpi_sdt_t *rsdt;
static acpi_sdt_t *madt;

static INITCALL acpi_init() {
    acpi_rsdp_t *rsdp;
    uint8_t *search = mm_map((void *) BIOS_DATA_START);

    uint8_t *page = (uint8_t *) (BIOS_DATA_START + PAGE_SIZE);
    for(uint32_t i = 1; i < BIOS_DATA_PAGES; i++) {
        mm_map(page);
        page += PAGE_SIZE;
    }

    uint32_t inc = 0;
    while(inc < (BIOS_DATA_END - BIOS_DATA_START)) {
        rsdp = (acpi_rsdp_t *) (search + inc);
        if(acpi_valid_rsdp(rsdp)) {
            break;
        }

        rsdp = NULL;
        inc += 16;
    }

    if(rsdp) {
        rsdt = mm_map(rsdp->rsdt);        
        if(acpi_sig_match(ACPI_SIG_RSDT, rsdt) && acpi_valid_sdt(rsdt)) {
            logf("acpi - rsdt load success");
            
            void **sdts = (void *) rsdt->data;
            for(uint32_t i = 0; i < (rsdt->len - sizeof(acpi_sdt_t)) / sizeof(void *); i++) {
                //FIXME unmap these pages
                acpi_sdt_t *real = mm_map(sdts[i]);
                if(acpi_valid_sdt(real)) {
                    if(acpi_sig_match(ACPI_SIG_MADT, real)) {
                        madt = real;
                    }
                }
            }
        } else {
            rsdt = NULL;

            logf("acpi - invalid rsdt");
        }
    } else {
        logf("acpi - no rsdp detected");
    }

    return 0;
}

arch_initcall(acpi_init);
