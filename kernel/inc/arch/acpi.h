#ifndef KERNEL_ARCH_ACPI_H
#define KERNEL_ARCH_ACPI_H

#include "lib/int.h"
#include "common/compiler.h"

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
} PACKED acpi_madt_t;

typedef struct gas {
    uint8_t addr_space_id;
    uint8_t reg_bit_width;
    uint8_t reg_bit_offset;
    uint8_t zero;
    uint64_t addr;
} PACKED gas_t;

void madt_parse(acpi_sdt_t *madt);

#endif
