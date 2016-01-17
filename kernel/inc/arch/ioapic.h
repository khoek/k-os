#ifndef KERNEL_ARCH_IOAPIC_H
#define KERNEL_ARCH_IOAPIC_H

#include <stdint.h>

void register_ioapic(uint8_t id, uint32_t base_addr);
void register_mp_bus(uint8_t id, char name[6]);
void register_ioint(uint16_t flags, uint8_t bus_id, uint8_t bus_irq, uint8_t ioapic_id, uint8_t intin);

uint8_t find_pci_ioint(uint8_t dev_num);

#endif
