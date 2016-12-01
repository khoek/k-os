#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include "common/compiler.h"
#include "sched/task.h"

extern volatile bool tasking_up;

void __noreturn sched_loop();

void thread_schedule(thread_t *task);
void thread_sleep_prepare();
void thread_wake(thread_t *task);

void sched_switch();
void sched_try_resched();

#define wait_for_condition(C)                                            \
    ({while(!(C)) {sched_suspend_pending_interrupt();};true;})

void sched_suspend_pending_interrupt();
void sched_interrupt_notify();

void deliver_signals();

#endif
