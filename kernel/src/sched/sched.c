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
#include "arch/pl.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "sched/sched.h"
#include "sched/proc.h"
#include "sched/task.h"
#include "fs/file/char.h"
#include "log/log.h"
#include "misc/stats.h"

#define QUANTUM 1000

#define SWITCH_INT 0x81

static volatile bool tasking_up = false;

static DEFINE_LIST(tasks);
static DEFINE_LIST(queued_tasks);

static DEFINE_SPINLOCK(sched_lock);

static DEFINE_PER_CPU(task_t *, idle_task);
static DEFINE_PER_CPU(uint64_t, switch_time);

void sched_switch() {
    asm volatile ("int $" XSTR(SWITCH_INT));
}

static void deactivate_current() {
    if(!current) return;

    task_t *task = current;

    spin_lock(&task->lock);

    switch(task->state) {
        case TASK_IDLE:
        case TASK_SLEEPING: {
            spin_unlock(&task->lock);

            break;
        }
        case TASK_RUNNING: {
            task->state = TASK_AWAKE;
            list_add_before(&task->queue_list, &queued_tasks);
            spin_unlock(&task->lock);

            break;
        }
        case TASK_EXITED: {
            spin_unlock(&task->lock);

            //TODO handle this

            break;
        }
        case TASK_AWAKE:
        default: {
            BUG();
        }
    }

    current = NULL;
}

static task_t * find_next_current() {
    while(!list_empty(&queued_tasks)) {
        task_t *task = list_first(&queued_tasks, task_t, queue_list);
        spin_lock(&task->lock);
        list_rm(&task->queue_list);

        switch(task->state) {
            case TASK_AWAKE: {
                get_percpu_unsafe(switch_time) = uptime() + QUANTUM;
                task->state = TASK_RUNNING;
                current = task;
                spin_unlock(&task->lock);

                return task;
            }
            case TASK_EXITED: {
                spin_unlock(&task->lock);

                //TODO handle this

                task = NULL;
                break;
            }
            default: {
                BUG();
            }
        }
    }

    //We ran out of queued tasks.
    get_percpu_unsafe(switch_time) = 0;
    return get_percpu_unsafe(idle_task);
}

static void do_sched_switch() {
    spin_lock_irq(&sched_lock);

    deactivate_current();
    task_t *newcurrent = find_next_current();
    current = newcurrent;

    //IRQs will remain disabled
    spin_unlock(&sched_lock);

    context_switch(newcurrent);
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
    BUG_ON(task == current && (get_eflags() & EFLAGS_IF));

    task->state = TASK_SLEEPING;
    if(task->state == TASK_AWAKE) {
        list_rm(&task->queue_list);
    }

    spin_unlock(&task->lock);
    spin_unlock_irqstore(&sched_lock, flags);
}

void task_sleep_current() {
    cli(); // This must be here, see task_sleep()

    task_sleep(current);
    sched_switch();

    sti();
}

static void do_wake(task_t *task) {
    spin_lock(&task->lock);

    task->state = TASK_AWAKE;
    list_add_before(&task->queue_list, &queued_tasks);

    spin_unlock(&task->lock);
}

void task_wake(task_t *task) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    do_wake(task);

    spin_unlock_irqstore(&sched_lock, flags);
}

void task_add(task_t *task) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    list_add_before(&task->list, &tasks);
    do_wake(task);

    spin_unlock_irqstore(&sched_lock, flags);
}

void task_save(void *sp) {
    if(tasking_up) {
        BUG_ON(!current);
        save_cpu_state(current, sp);
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
    get_percpu_unsafe(idle_task) = create_idle_task();

    if(get_percpu_unsafe(this_proc)->num == BSP_ID) {
        kprintf("sched - activating scheduler");

        tasking_up = true;
    } else {
        while(!tasking_up);
    }

    do_sched_switch();

    BUG();
}

static void switch_interrupt(interrupt_t *interrupt, void *data) {
    eoi_handler(interrupt->vector);
    cli();
    do_sched_switch();
}

static INITCALL sched_init() {
    register_isr(SWITCH_INT, CPL_KRNL, switch_interrupt, NULL);

    return 0;
}

subsys_initcall(sched_init);
