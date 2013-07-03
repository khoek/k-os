#ifndef KERNEL_CPL_H
#define KERNEL_CPL_H

#include "int.h"
#include "registers.h"

void cpl_switch(uint32_t cr3, registers_t registers, exec_state_t exec);

#endif
