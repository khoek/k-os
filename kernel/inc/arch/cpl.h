#ifndef KERNEL_ARCH_CPL_H
#define KERNEL_ARCH_CPL_H

#include "lib/int.h"
#include "arch/registers.h"

void cpl_switch(uint32_t cr3, uint32_t eax, uint32_t edx, uint32_t cpu);

#endif
