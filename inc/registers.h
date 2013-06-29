#ifndef KERNEL_REGISTERS_H
#define KERNEL_REGISTERS_H

#include "common.h"

typedef struct registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pusha/popa order
} PACKED registers_t;

typedef struct task_state {
    uint32_t eip, cs, eflags, esp, ss; //iret pop order
} PACKED task_state_t;

void flush_segment_registers();

#define EFLAGS_IF (1 << 9)

uint32_t get_eflags();

#endif
