#ifndef KERNEL_ARCH_CPU_H
#define KERNEL_ARCH_CPU_H

typedef struct cpu_state cpu_state_t;
typedef struct cpu_launchpad cpu_launchpad_t;

typedef struct arch_thread_data arch_thread_data_t;

#include "common/types.h"
#include "common/compiler.h"

static inline void * get_sp() {
    void *esp;
    asm("mov %%esp, %0" : "=r" (esp));
    return esp;
}

typedef uint32_t phys_addr_t;

typedef struct registers {
    //pusha/popa order
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} PACKED registers_t;

typedef struct exec_state {
    uint32_t eip, cs, eflags;
} PACKED exec_state_t;

typedef struct stack_state {
    uint32_t esp, ss;
} PACKED stack_state_t;

typedef struct cpu_state {
    //pusha saved register values
    registers_t reg;

    //iret pop order, esp and ss are not present if the interrupt occured
    //without a PL switch.
    exec_state_t exec;

    //These aren't present if a kernel PL task was interrupted
    stack_state_t stack;
} PACKED cpu_state_t;

typedef struct cpu_launchpad {
    registers_t reg;
    exec_state_t exec;
} PACKED cpu_launchpad_t;

typedef struct arch_thread_data {
    //Top of kernel stack (where we popa/iret during a task switch).
    cpu_state_t *live_state;

    phys_addr_t cr3;
    void *dir;
} arch_thread_data_t;

void flush_segment_registers();

#define EFLAGS_IF (1 << 9)

uint32_t get_eflags();
void set_eflags(uint32_t flags);

void enter_syscall();
void leave_syscall();

void save_stack(void *sp);

#include "sched/task.h"

void switch_stack(thread_t *old, thread_t *next, void (*finish_sched_switch)(thread_t *old, thread_t *next));

#endif
