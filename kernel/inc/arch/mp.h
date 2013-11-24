#ifndef KERNEL_ARCH_MP_H
#define KERNEL_ARCH_MP_H

#include "common/list.h"
#include "common/compiler.h"
#include "arch/acpi.h"

void __noreturn mp_ap_init();
void __init mp_init(acpi_sdt_t *madt);

#endif
