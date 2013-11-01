#include "init/initcall.h"
#include "lib/string.h"
#include "common/compiler.h"
#include "mm/mm.h"
#include "video/log.h"

#define ACPI_RSDP_SIG "RSD PTR "

#define BIOS_DATA_START (0xE0000)
#define BIOS_DATA_END   (0xFFFFF + 1)
#define BIOS_DATA_PAGES (DIV_UP(BIOS_DATA_END - BIOS_DATA_START, PAGE_SIZE))

typedef struct acpi_rsdp {
    uint8_t sig[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t rev;
    void *rsdt;
} PACKED acpi_rsdp_t;

typedef struct acpi_sdt {
    uint8_t sig[4];
    uint32_t len;
    uint8_t rev;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_tableid[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_rev;
} PACKED acpi_sdt_t;

static inline bool acpi_checksum(void *ptr, uint32_t bytes) {
    uint8_t sum = 0;
    for(uint32_t i = 0; i < bytes; i++) {
        sum += *(((uint8_t *) ptr) + i);
    }

    return !sum;
}

static inline bool acpi_valid_rsdp(acpi_rsdp_t *rsdp) {
    if(memcmp(ACPI_RSDP_SIG, rsdp->sig, sizeof(rsdp->sig))) return false;
    return acpi_checksum(rsdp, sizeof(acpi_rsdp_t));
}

static inline bool acpi_valid_sdt(acpi_sdt_t *sdt) {
    return acpi_checksum(sdt, sdt->len);
}

static acpi_sdt_t *rsdt;

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
        if(acpi_valid_sdt(rsdt)) {
            logf("acpi - rsdt load success");
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
