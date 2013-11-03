#ifndef KERNEL_ARCH_ACPI_H
#define KERNEL_ARCH_ACPI_H

#define MADT_ENTRY_TYPE_LAPIC 0

#define MADT_ENTRY_LAPIC_FLAG_ENABLED (1 << 0)

#include "lib/int.h"

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
    uint8_t data[];
} PACKED acpi_sdt_t;

typedef struct acpi_madt {
    void *lapic_base;
    uint32_t flags;
    uint8_t records[];
} PACKED acpi_madt_t;

extern acpi_sdt_t *rsdt;
extern acpi_sdt_t *madt;

#endif
