#ifndef KERNEL_ARCH_PL_H
#define KERNEL_ARCH_PL_H

#include "lib/int.h"
#include "arch/cpu.h"

void context_switch(task_t *t);

void pl_enter_userland(void *user_ip, void *user_stack);
void pl_setup_task(task_t *task, void *ip, void *arg);

#endif