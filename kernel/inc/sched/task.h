#ifndef KERNEL_SCHED_TASK_H
#define KERNEL_SCHED_TASK_H

#define UFDT_PAGES          1
#define UFD_LIST_PAGES      1
#define KERNEL_STACK_PAGES  16

#define KERNEL_STACK_LEN (KERNEL_STACK_PAGES * PAGE_SIZE)

#define TASK_RUNNING    0
#define TASK_EXITING    1
#define TASK_EXITED     2

#define THREAD_BUILDING 0
#define THREAD_AWAKE    1
#define THREAD_SLEEPING 2
#define THREAD_EXITED   3
#define THREAD_IDLE     4

typedef struct task_node task_node_t;
typedef struct thread thread_t;

#include "common/types.h"
#include "common/list.h"
#include "sync/spinlock.h"
#include "arch/cpu.h"
#include "arch/idt.h"
#include "sched/proc.h"
#include "sync/atomic.h"
#include "sync/semaphore.h"
#include "fs/fd.h"
#include "fs/vfs.h"

typedef int32_t ufd_idx_t;

typedef struct ufd {
    uint32_t refs;
    uint32_t flags;
    file_t *gfd;
} ufd_t;

typedef struct ufd_context {
    spinlock_t lock;

    uint32_t refs;

    ufd_idx_t next;
    ufd_t *table;
    ufd_idx_t *list;
} ufd_context_t;

typedef struct fs_context {
    spinlock_t lock;

    uint32_t refs;

    path_t root;
    path_t pwd;
} fs_context_t;

typedef struct task_node {
    spinlock_t lock;

    uint32_t refs;

    pid_t pid;
    task_node_t *parent;

    char **argv;
    char **envp;

    atomic_t exit_state;
    uint32_t exit_code;

    //List heads
    list_head_t threads;
    list_head_t children;
    list_head_t zombies;

    //for parent's children list
    list_head_t child_list;
    //for zombieing
    list_head_t zombie_list;
} task_node_t;

typedef struct thread {
    spinlock_t lock;

    //Inter-thread (shared) data:
    //FIXME these first two things should be associated to the task node!
    ufd_context_t *ufd;
    fs_context_t *fs;
    task_node_t *node;

    //Thread-specific data:
    bool active;
    uint32_t flags;
    void *kernel_stack_top;
    void *kernel_stack_bottom;

    //Thread-local data (read/write):
    int32_t state;
    uint32_t sig_pending;

    //processor this task is currently executing on, NULL otherwise
    arch_thread_data_t arch;

    //for global list "threads"
    list_head_t list;
    //for task_node list "threads"
    list_head_t thread_list;
    //for queueing and sleeping
    list_head_t state_list;
    //used in wait_for_condition()
    list_head_t poll_list;
} thread_t;

//FIXME delete the obtain_* functions because they aren't remotely thread safe

static inline task_node_t * obtain_task_node(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&t->lock, &flags);
    task_node_t *node = t->node;
    spin_unlock_irqstore(&t->lock, flags);
    return node;
}

static inline fs_context_t * obtain_fs_context(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&t->lock, &flags);
    fs_context_t *fs = t->fs;
    spin_unlock_irqstore(&t->lock, flags);
    return fs;
}

#include "mm/mm.h"

thread_t * create_idle_task();
void spawn_kernel_task(char *name, void (*main)(void *arg), void *arg);

thread_t * thread_fork(thread_t *t, uint32_t flags, void (*setup)(void *arg), void *arg);
void thread_exit();

void task_node_exit(int32_t code);

bool ufdt_valid(ufd_idx_t ufd);
ufd_idx_t ufdt_add(uint32_t flags, file_t *gfd);

file_t * ufdt_get(ufd_idx_t ufd);
void ufdt_put(ufd_idx_t ufd);
void ufdt_close(ufd_idx_t ufd);

void __init root_task_init(void *umain);

#endif
