#ifndef KERNEL_CPL_H
#define KERNEL_CPL_H

#include "int.h"
#include "registers.h"

void cpl_switch(uint32_t cr3, registers_t registers, proc_state_t proc);

#endif
