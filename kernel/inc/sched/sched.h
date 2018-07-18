#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include "common/compiler.h"
#include "sched/task.h"

extern volatile bool tasking_up;

void __noreturn sched_loop();

bool are_signals_pending(thread_t *thread);
bool should_abort_slow_io();

void pgroup_send_signal(pgroup_t *t, uint32_t sig);
void task_send_signal(task_node_t *t, uint32_t sig);

void thread_schedule(thread_t *task);
void thread_sleep_prepare();
void thread_wake(thread_t *task);
void thread_send_signal(thread_t *t, uint32_t sig);

void sched_switch();
void sched_try_resched();

#define wait_for_condition(C)                                            \
    ({while(!(C)) {sched_suspend_pending_interrupt();};true;})

void sched_suspend_pending_interrupt();
void sched_interrupt_notify();

void sched_deliver_signals(cpu_state_t *state);

#endif
