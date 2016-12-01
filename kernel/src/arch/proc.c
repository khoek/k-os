#include "common/types.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "sched/proc.h"
#include "arch/apic.h"
#include "arch/proc.h"
#include "sched/task.h"
#include "log/log.h"

volatile bool percpu_up = false;

DEFINE_PER_CPU(thread_t *, current_task);

void arch_setup_proc(processor_t *proc) {
    gdt_init(proc);
    idt_init();
}

thread_t * get_current() {
    return current;
}
