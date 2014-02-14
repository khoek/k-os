#include "common/compiler.h"
#include "sched/proc.h"

#ifndef KERNEL_ARCH_PROC_H
#define KERNEL_ARCH_PROC_H

typedef struct processor_arch processor_arch_t;

#define DEFINE_PER_CPU(type, name) \
    type __attribute__ ((section(".data.percpu"))) __percpu_##name

#define DECLARE_PER_CPU(type, name) \
    extern type __attribute__ ((section(".data.percpu"))) __percpu_##name

#define get_percpu_raw(ptr, name)                                                       \
    (*({                                                                                \
        uint8_t *__t2 = (void *) (ptr);                                                 \
        __t2 += ((uint32_t)  &__percpu_##name ) - ((uint32_t) &percpu_data_start);      \
        (typeof( __percpu_##name ) *) __t2;                                             \
    }))

#define get_percpu_unsafe(name)                                     \
        get_percpu_raw(get_percpu_ptr(), name)

#define get_percpu(name)            \
    (*({                            \
        cli();                      \
        &get_percpu_unsafe(name);   \
    }))

#define put_percpu(name) do {sti();} while(0)

#define current get_percpu_unsafe(current_task)

#include "lib/int.h"
#include "common/compiler.h"
#include "common/asm.h"
#include "sched/task.h"

DECLARE_PER_CPU(task_t *, current_task);

extern uint32_t percpu_data_start;
extern uint32_t percpu_data_end;

struct processor_arch {
    uint32_t apic_id;
    uint32_t acpi_id;
};

static inline void * get_percpu_ptr() {
    void *ptr;
    __asm__ volatile("mov %%gs:(0), %%eax" : "=a" (ptr));
    return ptr;
}

void arch_setup_proc(processor_t *proc);

#endif
