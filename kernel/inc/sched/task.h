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

//thread runs with kernel privilege level
#define THREAD_FLAG_KERNEL (1 << 0)
//thread has resulted from a fork, and execve() has not yet been called
//FIXME implement clearing this
#define THREAD_FLAG_FORKED (1 << 1)
//thread is waiting in a semaphore
#define THREAD_FLAG_INSEM  (1 << 2)

typedef struct task_node task_node_t;
typedef struct thread thread_t;
typedef struct psession psession_t;
typedef struct pgroup pgroup_t;

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
#include "user/signal.h"

typedef uint32_t ufd_idx_t;

typedef struct ufd {
    uint32_t flags;
    file_t *gfd;
} ufd_t;

typedef struct ufd_context {
    spinlock_t lock;
    uint32_t refs;

    ufd_t *table;
} ufd_context_t;

typedef struct fs_context {
    spinlock_t lock;

    uint32_t refs;

    path_t root;
    path_t pwd;
} fs_context_t;

typedef void (*sigtramp_t)(void *);

//FIXME add a layer of indirection using task_id structs, which stay alive to
//keep track of session leaders and the like while allowing the main task_node
//to be deallocated when/if that task dies.

typedef struct psession {
    spinlock_t lock;
    task_node_t *leader;
    list_head_t members;

    list_head_t list;
} psession_t;

typedef struct pgroup {
    spinlock_t lock;
    task_node_t *leader;
    list_head_t members;

    list_head_t list;
} pgroup_t;

//These codes are defined to be compatible with the libc
#define ECAUSE_RQST 0x00 //exit requested
#define ECAUSE_SIG  0x01 //exit due to signal
#define EFLAG_CORE  0x80  //exit included core dump

typedef struct task_node {
    spinlock_t lock;

    uint32_t refs;

    pid_t pid;
    task_node_t *parent;

    char **argv;
    char **envp;

    atomic_t exit_state;
    uint32_t exit_code;
    uint8_t exit_cause;

    sigtramp_t sigtramp;
    struct sigaction sigactions[NSIG];

    sigset_t sig_mask;

    psession_t *session;
    pgroup_t *pgroup;

    //List heads
    list_head_t threads;
    list_head_t children;
    list_head_t zombies;

    //FIXME implement sessions and PGs
    list_head_t psession_list; //session
    list_head_t pgroup_list;   //process group members
    list_head_t child_list;  //parent's children
    list_head_t zombie_list; //for zombieing

    list_head_t list; //for master task list
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
    sigset_t sig_pending;
    bool should_die;

    //architecture-specific execution state
    arch_thread_data_t arch;

    //for global list "threads"
    list_head_t list;
    //for task_node list "threads"
    list_head_t thread_list;
    //for queueing
    list_head_t queue_list;
    //for sleeping in semaphore
    list_head_t sleep_list;
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

void task_node_get(task_node_t *node);
void task_node_put(task_node_t *node);
task_node_t * task_node_find(pid_t pid);
void task_node_exit(uint32_t code, uint8_t exit_cause);

void session_create(task_node_t *t);
void session_add(task_node_t *new, psession_t *session);
void session_rm(task_node_t *new);
psession_t * session_find(pid_t pid);

void pgroup_create(task_node_t *t);
void pgroup_add(task_node_t *new, pgroup_t *pg);
void pgroup_rm(task_node_t *new);
pgroup_t * pgroup_find(pid_t pid);

bool ufdt_valid(ufd_idx_t ufd);
ufd_idx_t ufdt_add(file_t *gfd);
int32_t ufdt_replace(ufd_idx_t ufd, file_t *fd);
int32_t ufdt_close(ufd_idx_t ufd);

file_t * ufdt_get(ufd_idx_t ufd);
void ufdt_put(ufd_idx_t ufd);

void __init root_task_init(void *umain);

//FIXME FIXME HEADER SOUP

#include "sync/semaphore-post.h"

#endif
