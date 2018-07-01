#ifndef KERNEL_SCHED_SYSCALL_H
#define KERNEL_SCHED_SYSCALL_H

#include "arch/syscall.h"

#include "shared/syscall_decls.h"

syscall_t syscalls[MAX_SYSCALL] = {
#include "shared/syscall_ents.h"
};

#endif
