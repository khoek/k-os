#ifndef KERNEL_CPL_H
#define KERNEL_CPL_H

#include "int.h"
#include "registers.h"

void cpl_switch(uint32_t cr3, uint32_t cpu);

#endif
