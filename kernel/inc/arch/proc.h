#ifndef KERNEL_ARCH_PROC_H
#define KERNEL_ARCH_PROC_H

#include "common/compiler.h"
#include "common/types.h"

typedef struct processor_arch processor_arch_t;

struct processor_arch {
    uint32_t apic_id;
    uint32_t acpi_id;
};

#define DEFINE_PER_CPU(type, name) \
    type __attribute__ ((section(".data.percpu"))) __percpu_##name

#define DECLARE_PER_CPU(type, name) \
    extern type __attribute__ ((section(".data.percpu"))) __percpu_##name

#define get_percpu_raw(ptr, name)                                   \
    (*({                                                               \
        barrier();                                                     \
        uint8_t *__t2 = (void *) (ptr);                                \
        __t2 += ((uint32_t) &__percpu_##name ) - ((uint32_t) &percpu_data_start); \
        typeof( __percpu_##name ) *__t3 = &(*((typeof( __percpu_##name ) *) __t2));                        \
        barrier();                                                     \
        __t3;                                                          \
    }))

#define get_percpu_unsafe(name)                                     \
        (*({&get_percpu_raw(get_percpu_ptr(), name);}))

#define get_percpu(name)                                     \
        (*({check_irqs_disabled(); &get_percpu_raw(get_percpu_ptr(), name);}))

#define current get_percpu_unsafe(current_task)

#include "common/compiler.h"
#include "common/asm.h"
#include "common/list.h"
#include "log/log.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "sched/proc.h"
#include "sched/task.h"

DECLARE_PER_CPU(thread_t *, current_task);
DECLARE_PER_CPU(uint32_t, locks_held);
DECLARE_PER_CPU(list_head_t, lock_list);

extern uint32_t percpu_data_start;
extern uint32_t percpu_data_end;

static inline volatile void * get_percpu_ptr() {
    void *ptr;
    __asm__ volatile("mov %%gs:(0), %%eax" : "=a" (ptr));
    return ptr;
}

extern volatile bool percpu_up;

#define check_no_locks_held() do {                                         \
    if(get_percpu(locks_held)) {                                           \
        panicf("%X locks held, %X", get_percpu(locks_held), list_first(&get_percpu(lock_list), spinlock_t, list));             \
    }                                                                      \
} while(0)

void arch_setup_proc(processor_t *proc);

#endif
