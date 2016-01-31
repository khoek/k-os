#ifndef KERNEL_ARCH_CPU_H
#define KERNEL_ARCH_CPU_H

typedef struct registers registers_t;
typedef struct exec_state exec_state_t;
typedef struct stack_state stack_state_t;

typedef struct cpu_state cpu_state_t;
typedef struct cpu_launchpad cpu_launchpad_t;

typedef struct arch_task_data arch_task_data_t;

typedef uint32_t phys_addr_t;

#include "common/types.h"
#include "common/compiler.h"

struct registers {
    //pusha/popa order
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} PACKED;

struct exec_state {
    uint32_t eip, cs, eflags;
} PACKED;

struct stack_state {
    uint32_t esp, ss;
} PACKED;

struct cpu_state {
    //pusha saved register values
    registers_t reg;

    //iret pop order, esp and ss are not present if the interrupt occured
    //without a PL switch.
    exec_state_t exec;

    //These aren't present if a kernel PL task was interrupted
    stack_state_t stack;
} PACKED;

struct cpu_launchpad {
    registers_t reg;
    exec_state_t exec;
} PACKED;

struct arch_task_data {
    //On top of the kernel_stack, updated every interrupt
    cpu_state_t *live_state;

    phys_addr_t cr3;
    void *dir;

    //A copy of the cpu state pointed to by live_state. *live_state is
    //overwritten by cpu_state at context_switch() time. Thus, live_state should
    //never be read from but especially never written to, except by the state
    //save/restore mechanism.
    cpu_state_t cpu_state;
};

#include "sched/task.h"

void flush_segment_registers();

#define EFLAGS_IF (1 << 9)

uint32_t get_eflags();
void set_eflags(uint32_t flags);

void save_cpu_state(task_t *t, void *sp);

#endif
