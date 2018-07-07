#ifndef KERNEL_ARCH_PL_H
#define KERNEL_ARCH_PL_H

#include "common/types.h"
#include "arch/cpu.h"
#include "sched/task.h"

void context_switch(thread_t *t);

void pl_enter_userland(void *user_ip, void *user_stack, registers_t *reg);
void pl_bootstrap_userland(void *user_ip, void *user_stack, uint32_t argc,
    void *argv, void *envp);
void pl_setup_thread(thread_t *task, void *ip, void *arg);

void * arch_replace_mem(thread_t *t, void *newdir);
void arch_free_mem(void *dir);

void arch_thread_build(thread_t *t);
void arch_ret_from_fork(void *arg);
void * arch_prepare_fork(cpu_state_t *state);

#endif
