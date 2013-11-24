#include "lib/int.h"
#include "init/initcall.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/proc.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "sched/proc.h"
#include "video/log.h"

processor_t *bsp;
static DEFINE_LIST(procs);
DEFINE_PER_CPU(processor_t *, this_proc);

processor_t * register_proc(uint32_t num) {
    processor_t *proc = kmalloc(sizeof(processor_t));
    proc->num = num;
    proc->percpu_data = num ? map_page(page_to_phys(alloc_pages(DIV_UP(((uint32_t) &percpu_data_end) - ((uint32_t) &percpu_data_start), PAGE_SIZE), 0))) : &percpu_data_start;
    list_add(&proc->list, &procs);

    arch_setup_proc(proc);

    get_percpu_unsafe(this_proc) = proc;

    return proc;
}

static INITCALL proc_init() {
    bsp = register_proc(0);

    return 0;
}

arch_initcall(proc_init);
