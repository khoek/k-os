#ifndef KERNEL_ARCH_APIC_H
#define KERNEL_ARCH_APIC_H

#include "init/initcall.h"

#define APIC_CMD_TYPE_INIT    (0x5)
#define APIC_CMD_TYPE_STARTUP (0x6)

#define APIC_CMD_FLAG_ASSERT  (1 << 14)

uint32_t apic_get_id();
void apic_issue_command(uint8_t target_id, uint16_t type, uint32_t flags, uint8_t vector);

void __init apic_enable();
void __init apic_init(void *base);

#endif
