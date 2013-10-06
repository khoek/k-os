#ifndef KERNEL_TASK_TASK_H
#define KERNEL_TASK_TASK_H

#include "lib/int.h"
#include "common/list.h"
#include "sync/spinlock.h"
#include "arch/registers.h"
#include "arch/idt.h"
#include "fs/fd.h"
#include "fs/vfs.h"

typedef enum task_state {
    TASK_NONE,
    TASK_AWAKE,
    TASK_SLEEPING
} task_state_t;

typedef struct task {
    list_head_t list;
    list_head_t wait_list;

    uint32_t kernel_stack;
    uint32_t cpu; //On the top of the kernel stack, updated every interrupt
    uint32_t cr3;
    uint32_t *directory;

    uint64_t ret; //Return value (for syscalls)

    uint32_t pid;
    uint32_t flags;

    int32_t sleeps; //sleeps > 0 means should be sleeping

    spinlock_t lock;

    ufd_t *fd;
    ufd_idx_t *fd_list;
    ufd_idx_t fd_next;
    ufd_idx_t fd_count;
    spinlock_t fd_lock;

    task_state_t state;

    path_t root;
    path_t pwd;
} task_t;

#include "mm/mm.h"

extern task_t *current;

task_t * task_create(bool kernel, void *ip, void *sp);
void task_schedule(task_t *task);
void task_add_page(task_t *task, page_t *page);

void task_exit(task_t *task, int32_t code);

void task_sleep(task_t *task);
void task_sleep_current();

void task_wake(task_t *task);

ufd_idx_t ufdt_add(task_t *task, uint32_t flags, gfd_idx_t gfd);
gfd_idx_t ufdt_get(task_t *task, ufd_idx_t ufd);
void ufdt_put(task_t *task, ufd_idx_t ufd);

void task_save(cpu_state_t *cpu);

void task_reschedule();
void task_run_scheduler();

void task_run();

#endif
