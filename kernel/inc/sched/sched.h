#ifndef KERNEL_SCHED_SCHED_H
#define KERNEL_SCHED_SCHED_H

#include "lib/int.h"
#include "common/list.h"
#include "common/compiler.h"
#include "sync/spinlock.h"
#include "arch/registers.h"
#include "arch/idt.h"
#include "arch/proc.h"
#include "sched/task.h"
#include "sched/proc.h"
#include "fs/fd.h"
#include "fs/vfs.h"
#include "mm/mm.h"

void __noreturn sched_loop();

void task_add(task_t *task);
void task_exit(task_t *task, int32_t code);
void task_sleep(task_t *task);
void task_sleep_current();
void task_wake(task_t *task);

void sched_reschedule();
void sched_try_resched();

void sched_run();

#endif
