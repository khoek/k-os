#ifndef KERNEL_ARCH_HPET_H
#define KERNEL_ARCH_HPET_H

#define MADT_ENTRY_TYPE_LAPIC  0
#define MADT_ENTRY_TYPE_IOAPIC 1

#define MADT_ENTRY_LAPIC_FLAG_ENABLED (1 << 0)

#include "lib/int.h"
#include "init/initcall.h"
#include "common/compiler.h"
#include "arch/acpi.h"

typedef struct hpet_data {
    uint32_t hpet_block_id;
    gas_t base;
    uint8_t num;
    uint16_t main_counter_min;
    uint8_t page_protection_oem;
} PACKED hpet_data_t;

void __init hpet_init(acpi_sdt_t *hpet);

#endif