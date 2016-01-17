#include "lib/int.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "sched/proc.h"
#include "arch/apic.h"
#include "arch/proc.h"
#include "log/log.h"

DEFINE_PER_CPU(task_t *, current_task);

void arch_setup_proc(processor_t *proc) {
    gdt_init(proc);
    idt_init();
}
