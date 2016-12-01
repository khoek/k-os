#ifndef KERNEL_SCHED_PROC_H
#define KERNEL_SCHED_PROC_H

typedef struct processor processor_t;

#include "common/types.h"
#include "common/list.h"
#include "arch/proc.h"

#define MANAGEMENT_INT 0xC0

#define BSP_ID 0

struct processor {
    uint32_t num;

    processor_arch_t arch;
    void *percpu_data;

    list_head_t list;
};

extern processor_t *bsp;

DECLARE_PER_CPU(processor_t *, this_proc);

void send_management_interrupt(processor_t *dest);
void dispatch_management_interrupts();

processor_t * register_proc(uint32_t num);

#endif
