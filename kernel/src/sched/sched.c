#include <stdbool.h>

#include "lib/int.h"
#include "lib/string.h"
#include "common/list.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/cpl.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "sched/sched.h"
#include "sched/proc.h"
#include "sched/task.h"
#include "fs/file/char.h"
#include "video/log.h"
#include "misc/stats.h"

#define QUANTUM 1000

#define SWITCH_INT 0x81

static volatile bool tasking_up = false;

static DEFINE_LIST(tasks);
static DEFINE_LIST(queued_tasks);

static DEFINE_SPINLOCK(sched_lock);

static DEFINE_PER_CPU(task_t *, idle_task);
static DEFINE_PER_CPU(uint64_t, switch_time);

static void idle_loop() {
    while(true) hlt();
}

void sched_switch() {
    asm volatile ("int $" XSTR(SWITCH_INT));
}

static void do_sched_switch() {
    spin_lock_irq(&sched_lock);

    if(current) {
        spin_lock(&current->lock);

        if(current->state == TASK_EXITED) {
            spin_unlock(&current->lock);

            kfree(current, sizeof(task_t));
        } else {
            if(current->state == TASK_RUNNING) {
                current->state = TASK_AWAKE;
                list_add_before(&current->queue_list, &queued_tasks);
            }

            spin_unlock(&current->lock);
        }
    }

    while(true) {
        if(list_empty(&queued_tasks)) {
            current = NULL;
            goto run_task;
        }

        task_t *task = list_first(&queued_tasks, task_t, queue_list);
        spin_lock(&task->lock);
        list_rm(&task->queue_list);

        if(task->state != TASK_AWAKE) {
            if(task->state == TASK_EXITED) {
                task_destroy(task);
            }
        } else {
            current = task;
            get_percpu_unsafe(switch_time) = uptime() + QUANTUM;
            current->state = TASK_RUNNING;

            spin_unlock(&task->lock);
            break;
        }

        spin_unlock(&task->lock);
    }

run_task:
    spin_unlock(&sched_lock);

    if(!current) {
        get_percpu_unsafe(switch_time) = 0;
        current = get_percpu_unsafe(idle_task);
    }

    tss_set_stack(current->kernel_stack);
    cpl_switch(current->cr3, (uint32_t) current->ret, (uint32_t) (current->ret >> 32), current->cpu);
}

void task_add(task_t *task) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    list_add_before(&task->list, &tasks);
    list_add_before(&task->queue_list, &queued_tasks);

    spin_unlock_irqstore(&sched_lock, flags);
}

void task_sleep(task_t *task) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);
    spin_lock(&task->lock);

    // If task_sleep(task=current) succeeds, excecution will stop at the next
    // interrupt (after an arbitrary length of time). This results in a race
    // condition, with unpredictable behaviour.
    //
    // task_sleep_current() is guaranteed to switch safely.
    if(task == current && (get_eflags() & EFLAGS_IF)) panic("task_sleep(task=current) in interruptible context!");

    task->state = TASK_SLEEPING;
    list_rm(&task->queue_list);

    spin_unlock(&task->lock);
    spin_unlock_irqstore(&sched_lock, flags);
}

void task_sleep_current() {
    cli(); // This must be here, see task_sleep()

    task_sleep(current);
    sched_switch();

    sti();
}

void task_wake(task_t *task) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);
    spin_lock(&task->lock);

    task->state = TASK_AWAKE;
    list_move_before(&task->queue_list, &queued_tasks);

    spin_unlock(&task->lock);
    spin_unlock_irqstore(&sched_lock, flags);
}

void task_exit(task_t *task, int32_t code) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    spin_lock(&task->lock);
    task->state = TASK_EXITED;
    spin_unlock(&task->lock);

    list_rm(&task->list);
    list_rm(&task->queue_list);

    task_count--;

    //TODO propagate the exit code somehow

    for(ufd_idx_t i = 0; i < task->fd_count - 1; i++) {
        if(ufdt_valid(task, i)) {
            ufdt_put(task, i);
        }
    }

    //TODO free task page directory, iterate through it and free all of the non-kernel page tables

    spin_unlock_irqstore(&sched_lock, flags);

    sched_switch();
}

void task_save(cpu_state_t *cpu) {
    if(current && tasking_up) {
        current->cpu = (uint32_t) cpu;
        current->ret = (((uint64_t) cpu->reg.edx) << 32) | ((uint64_t) cpu->reg.eax);
    }
}

void sched_try_resched() {
    if(tasking_up && (!current || get_percpu_unsafe(switch_time) <= uptime() || current->state != TASK_RUNNING)) {
        sched_switch();
    }
}

void __noreturn sched_loop() {
    kprintf("sched - proc #%u is READY", get_percpu_unsafe(this_proc)->num);

    current = NULL;
    get_percpu_unsafe(switch_time) = 0;
    get_percpu_unsafe(idle_task) = task_create("idle", true, idle_loop, NULL);

    if(get_percpu_unsafe(this_proc)->num == BSP_ID) {
        tasking_up = true;

        kprintf("sched - activating scheduler");
    } else {
        while(!tasking_up);
    }

    do_sched_switch();

    panic("sched_switch returned");
}

static void switch_interrupt(interrupt_t *interrupt, void *data) {
    eoi_handler(interrupt->vector);
    do_sched_switch();
}

static INITCALL sched_init() {
    register_isr(SWITCH_INT, CPL_KRNL, switch_interrupt, NULL);

    return 0;
}

subsys_initcall(sched_init);
