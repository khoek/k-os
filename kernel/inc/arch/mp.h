#ifndef KERNEL_ARCH_MP_H
#define KERNEL_ARCH_MP_H

#include <stdbool.h>

#include "common/list.h"
#include "common/compiler.h"
#include "arch/acpi.h"

extern volatile bool ap_starting;
extern volatile uint32_t next_ap_acpi_id;
extern volatile void *next_ap_stack;

extern uint32_t entry_ap_start;
void entry_ap();
extern uint32_t entry_ap_end;

void __noreturn mp_ap_init();
void __init mp_init();

#endif
