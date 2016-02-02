#ifndef KERNEL_ARCH_PL_H
#define KERNEL_ARCH_PL_H

#include "common/types.h"
#include "arch/cpu.h"

void context_switch(task_t *t);

void pl_enter_userland(void *user_ip, void *user_stack, registers_t *reg);
void pl_setup_task(task_t *task, void *ip, void *arg);

#endif
