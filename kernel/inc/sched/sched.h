#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include "common/compiler.h"
#include "sched/task.h"

void __noreturn sched_loop();

void task_add(task_t *task);
void task_sleep(task_t *task);
void task_sleep_current();
void task_wake(task_t *task);

void sched_switch();
void sched_try_resched();

#endif
