#ifndef KERNEL_SCHED_TASK_H
#define KERNEL_SCHED_TASK_H

#define UFDT_PAGES          1
#define UFD_LIST_PAGES      1
#define KERNEL_STACK_PAGES  16

#define KERNEL_STACK_LEN (KERNEL_STACK_PAGES * PAGE_SIZE)

typedef enum task_state {
    TASK_BUILDING,
    TASK_AWAKE,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_IDLE,
    TASK_EXITED,
} task_state_t;

typedef struct task task_t;

#include "lib/int.h"
#include "common/list.h"
#include "sync/spinlock.h"
#include "arch/cpu.h"
#include "arch/idt.h"
#include "fs/fd.h"
#include "fs/vfs.h"

typedef int32_t ufd_idx_t;

typedef struct ufd {
    uint32_t refs;
    uint32_t flags;
    file_t *gfd;
} ufd_t;

struct task {
    list_head_t list;
    list_head_t wait_list;
    list_head_t queue_list;

    char **argv;
    char **envp;

    void *kernel_stack_top;
    void *kernel_stack_bottom;

    uint32_t pid;
    uint32_t flags;

    spinlock_t lock;

    ufd_idx_t fd_next;
    spinlock_t fd_lock;

    ufd_t *ufdt;
    ufd_idx_t *ufd_list;

    task_state_t state;

    path_t root;
    path_t pwd;

    arch_task_data_t arch;
};

#include "mm/mm.h"

task_t * create_idle_task();
void spawn_kernel_task(void (*main)(void *arg), void *arg);

void task_save(void *sp);

void task_fork(uint32_t flags, void (*setup)(void *arg), void *arg);
void task_exit(int32_t code);

void copy_fds(task_t *to, task_t *from);

bool ufdt_valid(ufd_idx_t ufd);
ufd_idx_t ufdt_add(uint32_t flags, file_t *gfd);

file_t * ufdt_get(ufd_idx_t ufd);
void ufdt_put(ufd_idx_t ufd);
void ufdt_close(ufd_idx_t ufd);

void __init root_task_init(void *umain, char **argv, char **envp);

#endif
