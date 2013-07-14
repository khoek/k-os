#ifndef KERNEL_ARCH_REGISTERS_H
#define KERNEL_ARCH_REGISTERS_H

#include "lib/int.h"
#include "common/compiler.h"

typedef struct registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pusha/popa order
} PACKED registers_t;

typedef struct exec_state {
    uint32_t eip, cs, eflags, esp, ss; //iret pop order, esp and ss are invalid if the interrupt occured within the kernel
} PACKED exec_state_t;

typedef struct cpu_state {
    registers_t reg;
    exec_state_t exec;
} PACKED cpu_state_t;

void flush_segment_registers();

#define EFLAGS_IF (1 << 9)

uint32_t get_eflags();

#endif
