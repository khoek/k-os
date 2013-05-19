#include "common.h"

typedef struct registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; //pusha/popa order
} PACKED registers_t;

typedef struct state {
    uint32_t eip, cs, eflags, esp, ss; //iret pop order
} PACKED state_t;

void flush_segment_registers();
void flush_tss();
void enter_usermode();
