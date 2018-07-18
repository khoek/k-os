#include <stdbool.h>

#include "common/types.h"
#include "lib/string.h"
#include "common/list.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/pl.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "sched/sched.h"
#include "sched/proc.h"
#include "sched/task.h"
#include "log/log.h"
#include "misc/stats.h"

#define QUANTUM 100

#define SWITCH_INT 0x81

#define UFD_INVALID ((ufd_idx_t) -1)
#define MAX_NUM_FDS MIN(((ufd_idx_t) (PAGE_SIZE / sizeof(ufd_t))), UFD_INVALID)

static pid_t pid = 0;

static cache_t *thread_cache;
static task_node_t *init_node;
static task_node_t *idle_node;

volatile bool tasking_up = false;

static DEFINE_LIST(threads);
static DEFINE_LIST(queued_threads);

static DEFINE_LIST(tasks);
static DEFINE_LIST(sessions);
static DEFINE_LIST(pgroups);

//Must be aquired before any task locks, used when manipulating the queue.
//It must be held whenever the heirachy is being walked.
static DEFINE_SPINLOCK(sched_lock);
static DEFINE_SPINLOCK(session_lock);
static DEFINE_SPINLOCK(pgroup_lock);

static DEFINE_PER_CPU(thread_t *, idle_task);
static DEFINE_PER_CPU(uint64_t, switch_time);

#define SD_NONE 0

//cannot be caught or ignored
#define SD_UNCATCHABLE (1 << 0)
//should be delivered to kernel-mode threads (by default none are)
#define SD_KERNEL      (1 << 1)

typedef struct sig_descriptor {
    uint32_t num;
    uint32_t flags;
    void (*handler)();
} sig_descriptor_t;

static struct sigaction default_sigactions[NSIG];
static sig_descriptor_t signals[NSIG];

static bool __ufdt_is_present(ufd_context_t *ufds, ufd_idx_t ufd);

void sched_switch() {
    uint32_t flags;
    irqsave(&flags);
    asm volatile ("int $" XSTR(SWITCH_INT));
    irqstore(flags);
}

//Returns true if task_node is not exiting, holding the task_node->lock. If
//false the lock is not held.
static inline bool task_node_ensure_not_exiting(task_node_t *node) {
    int32_t exit_state;

    //You aren't allowed to hold any thread locks when aquiring the task node.
    while(!spin_trylock(&node->lock)) {
        exit_state = atomic_read(&node->exit_state);
        if(exit_state != TASK_RUNNING) {
            return false;
        }
    }

    exit_state = atomic_read(&node->exit_state);
    if(exit_state != TASK_RUNNING) {
        spin_unlock(&node->lock);
        return false;
    }

    return true;
}

static inline ufd_context_t * ufd_context_build() {
    ufd_context_t *ufd = kmalloc(sizeof(ufd_context_t));
    ufd->refs = 1;
    ufd->table = kmalloc(UFDT_PAGES * PAGE_SIZE);
    spinlock_init(&ufd->lock);

    for(ufd_idx_t i = 0; i < MAX_NUM_FDS - 1; i++) {
        ufd->table[i].gfd = NULL;
        ufd->table[i].flags = 0;
    }

    return ufd;
}

static inline void ufd_context_destroy(ufd_context_t *ufd) {
    for(uint32_t i = 0; i < MAX_NUM_FDS; i++) {
        if(__ufdt_is_present(ufd, i)) {
            gfdt_put(ufd->table[i].gfd);
        }
    }

    kfree(ufd->table);
    kfree(ufd);
}

static inline ufd_context_t * ufd_context_dup(ufd_context_t *src) {
    ufd_context_t *dst = kmalloc(sizeof(ufd_context_t));
    dst->refs = 1;
    dst->table = kmalloc(UFDT_PAGES * PAGE_SIZE);
    spinlock_init(&dst->lock);

    uint32_t flags;
    spin_lock_irqsave(&src->lock, &flags);

    for(ufd_idx_t i = 0; i < MAX_NUM_FDS - 1; i++) {
        if(__ufdt_is_present(src, i)) {
            gfdt_get(src->table[i].gfd);
        }

        dst->table[i].gfd = src->table[i].gfd;
        dst->table[i].flags = src->table[i].flags;
    }

    spin_unlock_irqstore(&src->lock, flags);

    return dst;
}

static inline fs_context_t * fs_context_build() {
    fs_context_t *fs = kmalloc(sizeof(fs_context_t));
    fs->refs = 1;
    fs->root = MNT_ROOT(root_mount);
    fs->pwd = MNT_ROOT(root_mount);
    spinlock_init(&fs->lock);

    return fs;
}

static inline fs_context_t * fs_context_dup(fs_context_t *src) {
    fs_context_t *dst = kmalloc(sizeof(fs_context_t));
    dst->refs = 1;
    spinlock_init(&dst->lock);

    uint32_t flags;
    spin_lock_irqsave(&src->lock, &flags);

    dst->root = src->root;
    dst->pwd = src->pwd;

    spin_unlock_irqstore(&src->lock, flags);

    return dst;
}

static inline void fs_context_destroy(fs_context_t *fs) {
    kfree(fs);
}

static inline void put_fs_context(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&t->lock, &flags);
    fs_context_t *fs = t->fs;
    t->fs = NULL;
    spin_unlock_irqstore(&t->lock, flags);

    if(fs) {
        uint32_t flags;
        spin_lock_irqsave(&fs->lock, &flags);
        uint32_t newrefs = fs->refs--;
        spin_unlock_irqstore(&fs->lock, flags);

        if(!newrefs) {
            fs_context_destroy(fs);
        }
    }
}

task_node_t * task_node_find(pid_t pid) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    bool found = false;
    task_node_t *t;
    LIST_FOR_EACH_ENTRY(t, &tasks, list) {
        spin_lock(&t->lock);
        if(t->pid == pid) {
            found = true;
        }
        spin_unlock(&t->lock);

        if(found) break;
    }

    task_node_get(t);

    spin_unlock_irqstore(&sched_lock, flags);

    if(!found) {
        return NULL;
    }

    return t;
}

void session_create(task_node_t *t) {
    psession_t *session = kmalloc(sizeof(psession_t));
    session->leader = t;
    spinlock_init(&session->lock);
    list_init(&session->members);

    uint32_t flags;
    spin_lock_irqsave(&session_lock, &flags);

    session_add(t, session);
    list_add(&session->list, &sessions);

    spin_unlock_irqstore(&session_lock, flags);
}

void session_add(task_node_t *new, psession_t *session) {
    task_node_get(session->leader);
    new->session = session;

    uint32_t flags;
    spin_lock_irqsave(&session->lock, &flags);
    list_add(&new->psession_list, &session->members);
    spin_unlock_irqstore(&session->lock, flags);
}

void session_rm(task_node_t *new) {
    psession_t *session = new->session;

    uint32_t flags;
    spin_lock_irqsave(&session->lock, &flags);
    list_rm(&new->psession_list);
    spin_unlock_irqstore(&session->lock, flags);

    task_node_put(session->leader);
}

psession_t * session_find(pid_t pid) {
    uint32_t flags;
    spin_lock_irqsave(&session_lock, &flags);

    bool found = false;
    psession_t *session;
    LIST_FOR_EACH_ENTRY(session, &sessions, list) {
        spin_lock(&session->lock);
        if(session->leader->pid == pid) {
            found = true;
        }
        spin_unlock(&session->lock);

        if(found) break;
    }

    spin_unlock_irqstore(&session_lock, flags);

    return found ? session : NULL;
}

void pgroup_create(task_node_t *t) {
    pgroup_t *pg = kmalloc(sizeof(pgroup_t));
    pg->leader = t;
    spinlock_init(&pg->lock);
    list_init(&pg->members);

    uint32_t flags;
    spin_lock_irqsave(&pgroup_lock, &flags);

    pgroup_add(t, pg);
    list_add(&pg->list, &pgroups);

    spin_unlock_irqstore(&pgroup_lock, flags);
}

void pgroup_add(task_node_t *new, pgroup_t *pg) {
    task_node_get(pg->leader);
    new->pgroup = pg;

    uint32_t flags;
    spin_lock_irqsave(&pg->lock, &flags);
    list_add(&new->pgroup_list, &pg->members);
    spin_unlock_irqstore(&pg->lock, flags);
}

void pgroup_rm(task_node_t *new) {
    pgroup_t *pg = new->pgroup;

    uint32_t flags;
    spin_lock_irqsave(&pg->lock, &flags);
    list_rm(&new->pgroup_list);
    spin_unlock_irqstore(&pg->lock, flags);

    //FIXME if this emtpy now, delete it

    task_node_put(pg->leader);
}

pgroup_t * pgroup_find(pid_t pid) {
    uint32_t flags;
    spin_lock_irqsave(&pgroup_lock, &flags);

    bool found = false;
    pgroup_t *pg;
    LIST_FOR_EACH_ENTRY(pg, &pgroups, list) {
        spin_lock(&pg->lock);
        if(pg->leader->pid == pid) {
            found = true;
        }
        spin_unlock(&pg->lock);

        if(found) break;
    }

    spin_unlock_irqstore(&pgroup_lock, flags);

    return found ? pg : NULL;
}

static inline task_node_t * task_node_build(task_node_t *parent, char **argv,
        char **envp) {
    task_node_t *node = kmalloc(sizeof(task_node_t));
    node->refs = 1;
    node->pid = pid++;
    node->argv = (argv || !parent) ? argv : parent->argv;
    node->envp = (envp || !parent) ? envp : parent->envp;
    node->parent = parent;
    atomic_set(&node->exit_state, TASK_RUNNING);
    spinlock_init(&node->lock);

    node->sigtramp = (sigtramp_t) NULL;
    for(uint32_t i = 0; i < NSIG; i++) {
        node->sigactions[i] = default_sigactions[i];
    }

    list_init(&node->threads);
    list_init(&node->children);
    list_init(&node->zombies);

    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    if(parent) {
        spin_lock(&parent->lock);
        list_add(&node->child_list, &parent->children);
        spin_unlock(&parent->lock);

        session_add(node, parent->session);
        pgroup_add(node, parent->pgroup);
    } else {
        session_create(node);
        pgroup_create(node);
    }

    list_add(&node->list, &tasks);

    spin_unlock_irqstore(&sched_lock, flags);

    return node;
}

static void task_node_destroy(task_node_t *node) {
    panic("task_node_destroy()");
    kfree(node);
}

//assumed that we have at least one refcnt on node to begin with, else this is
//crazy
void task_node_get(task_node_t *node) {
    uint32_t flags;
    spin_lock_irqsave(&node->lock, &flags);

    BUG_ON(!node->refs);
    node->refs++;

    spin_unlock_irqstore(&node->lock, flags);
}

//assumed we have a lock on node (else this is crazy)
void task_node_put(task_node_t *node) {
    uint32_t flags;
    spin_lock_irqsave(&node->lock, &flags);

    BUG_ON(!node->refs);
    node->refs--;

    if(!node->refs) {
        task_node_destroy(node);
    }

    spin_unlock_irqstore(&node->lock, flags);
}

//Invoked under node->lock.
static inline void task_node_zombify(task_node_t *node) {
    //TODO when reaped invoke destroy

    atomic_set(&node->exit_state, TASK_EXITED);

    if(node->pid == 1) {
        panic("init exited");
    }

    //Zombify ourself
    task_node_t *parent = node->parent;
    if(task_node_ensure_not_exiting(parent)) {
        list_add(&node->zombie_list, &parent->zombies);
        spin_unlock(&parent->lock);
    }

    //Reparent children
    task_node_t *child;
    LIST_FOR_EACH_ENTRY(child, &node->children, child_list) {
        spin_lock(&child->lock);

        int32_t exit_state = atomic_read(&node->exit_state);
        if(exit_state == TASK_EXITED) {
            spin_lock(&init_node->lock);
            list_add(&child->zombie_list, &init_node->zombies);
            spin_unlock(&init_node->lock);
        } else {
            child->parent = init_node;
        }

        spin_unlock(&child->lock);
    }
}

static inline ufd_context_t * obtain_ufds(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&t->lock, &flags);
    ufd_context_t *ufd = t->ufd;
    spin_unlock_irqstore(&t->lock, flags);
    return ufd;
}

static inline ufd_context_t * get_ufds(thread_t *t) {
    ufd_context_t *ufd = obtain_ufds(t);

    uint32_t flags;
    spin_lock_irqsave(&ufd->lock, &flags);
    if(ufd) {
        ufd->refs++;
    }
    spin_unlock_irqstore(&ufd->lock, flags);

    return ufd;
}

static inline void put_ufds(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&t->lock, &flags);
    ufd_context_t *ufd = t->ufd;
    t->ufd = NULL;
    spin_unlock_irqstore(&t->lock, flags);

    if(ufd) {
        uint32_t flags;
        spin_lock_irqsave(&ufd->lock, &flags);
        uint32_t newrefs = ufd->refs--;
        spin_unlock_irqstore(&ufd->lock, flags);

        if(!newrefs) {
            ufd_context_destroy(ufd);
        }
    }
}

static inline task_node_t * get_task_node(thread_t *t) {
    task_node_t *node = obtain_task_node(t);

    if(node) {
        spin_lock(&node->lock);
        node->refs++;
        spin_unlock(&node->lock);
    }
    return node;
}

static inline void put_task_node(thread_t *t) {
    task_node_t *node = obtain_task_node(t);
    if(node) {
        uint32_t flags;
        spin_lock_irqsave(&node->lock, &flags);
        node->refs--;
        spin_unlock_irqstore(&node->lock, flags);
    }
}

static inline thread_t * thread_build(task_node_t *node, ufd_context_t *ufd,
        fs_context_t *fs) {
    thread_t *thread = cache_alloc(thread_cache);
    thread->state = THREAD_BUILDING;
    thread->should_die = false;
    sigemptyset(&thread->sig_pending);

    thread->node = node;
    thread->ufd = ufd;
    thread->fs = fs;
    thread->flags = THREAD_FLAG_KERNEL; //every thread starts of in kernel land
    thread->active = false;
    thread->kernel_stack_top = kmalloc(KERNEL_STACK_LEN);
    thread->kernel_stack_bottom = thread->kernel_stack_top + KERNEL_STACK_LEN;
    spinlock_init(&thread->lock);

    list_add(&thread->thread_list, &node->threads);

    arch_thread_build(thread);

    return thread;
}

void spawn_kernel_task(char *name, void (*main)(void *arg), void *arg) {
    thread_t *child = thread_fork(current, 0, main, arg);
    char **argv = kmalloc(2 * sizeof(char *));
    argv[0] = kmalloc(strlen(name) + 1);
    strcpy(argv[0], name);
    argv[1] = NULL;
    obtain_task_node(child)->argv = argv;
}

thread_t * thread_fork(thread_t *t, uint32_t flags, void (*setup)(void *arg),
        void *arg) {
    ufd_context_t *ufd = ufd_context_dup(obtain_ufds(t));
    fs_context_t *fs = fs_context_dup(obtain_fs_context(t));
    task_node_t *node = task_node_build(obtain_task_node(t), NULL, NULL);

    thread_t *child = thread_build(node, ufd, fs);
    child->flags |= THREAD_FLAG_FORKED;
    pl_setup_thread(child, setup, arg);

    copy_mem(child, t);

    thread_schedule(child);

    return child;
}

void __init root_task_init(void *umain) {
    thread_count++;

    ufd_context_t *ufd = ufd_context_build();
    fs_context_t *fs = fs_context_build();

    thread_t *init = thread_build(init_node, ufd, fs);
    pl_setup_thread(init, umain, NULL);

    thread_schedule(init);

    kprintf("task - root task created");
}

static void idle_loop(void *UNUSED(arg)) {
    irqenable();

    while(true) hlt();
}

thread_t * create_idle_task() {
    thread_t *idler = thread_build(idle_node, NULL, NULL);
    pl_setup_thread(idler, idle_loop, NULL);
    idler->state = THREAD_IDLE;
    return idler;
}

//FIXME XXX we need to be getting and putting the underlying file in all of the
//ufdt_*() functions below.

static bool __ufdt_is_present(ufd_context_t *ufds, ufd_idx_t ufd) {
    return ufd < MAX_NUM_FDS && ufds->table[ufd].gfd;
}

static ufd_idx_t __ufdt_find_next(ufd_context_t *ufds) {
    for(uint32_t idx = 0; idx < MAX_NUM_FDS; idx++) {
        if(!__ufdt_is_present(ufds, idx)) {
            return idx;
        }
    }
    return UFD_INVALID;
}

static int32_t __ufdt_install(ufd_context_t *ufds, ufd_idx_t idx, file_t *gfd) {
    gfdt_get(gfd);
    ufds->table[idx].gfd = gfd;
    ufds->table[idx].flags = 0;

    return 0;
}

ufd_idx_t ufdt_add(file_t *gfd) {
    ufd_context_t *ufds = obtain_ufds(current);

    uint32_t f;
    spin_lock_irqsave(&ufds->lock, &f);

    ufd_idx_t added = __ufdt_find_next(ufds);
    if(added != UFD_INVALID) {
        __ufdt_install(ufds, added, gfd);
    }

    spin_unlock_irqstore(&ufds->lock, f);

    return added;
}

static int32_t __ufdt_close(ufd_context_t *ufds, ufd_idx_t ufd) {
    gfdt_put(ufds->table[ufd].gfd);
    ufds->table[ufd].gfd = NULL;
    ufds->table[ufd].flags = 0;

    return 0;
}

int32_t ufdt_close(ufd_idx_t ufd) {
    int32_t ret = 0;
    ufd_context_t *ufds = obtain_ufds(current);

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    if(!__ufdt_is_present(ufds, ufd)) {
        ret = -EBADF;
        goto out;
    }
    ret = __ufdt_close(ufds, ufd);

out:
    spin_unlock_irqstore(&ufds->lock, flags);
    return ret;
}

//close if open, then install
int32_t ufdt_replace(ufd_idx_t ufd, file_t *fd) {
    int32_t ret;
    ufd_context_t *ufds = obtain_ufds(current);

    uint32_t f;
    spin_lock_irqsave(&ufds->lock, &f);

    if(__ufdt_is_present(ufds, ufd)) {
        ret = __ufdt_close(ufds, ufd);
        if(ret) {
            goto out;
        }
    }

    ret = __ufdt_install(ufds, ufd, fd);

out:
    spin_unlock_irqstore(&ufds->lock, f);
    return ret;
}

//FIXME this function _is_ a race!!
bool ufdt_valid(ufd_idx_t ufd) {
    if(ufd >= MAX_NUM_FDS) {
        return false;
    }

    ufd_context_t *ufds = obtain_ufds(current);

    bool response;

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    response = !!ufds->table[ufd].gfd;

    spin_unlock_irqstore(&ufds->lock, flags);

    return response;
}

file_t * ufdt_get(ufd_idx_t ufd) {
    ufd_context_t *ufds = obtain_ufds(current);

    file_t *gfd = NULL;

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    if(__ufdt_is_present(ufds, ufd)) {
        gfd = ufds->table[ufd].gfd;

        //inc gfd refcnt
    }

    spin_unlock_irqstore(&ufds->lock, flags);

    return gfd;
}

void ufdt_put(ufd_idx_t ufd) {
    ufd_context_t *ufds = obtain_ufds(current);

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    //FIXME dec the count of the underlying file

    spin_unlock_irqstore(&ufds->lock, flags);
}

void thread_sleep_prepare() {
    check_irqs_disabled();

    thread_t *me = current;

    spin_lock(&me->lock);

    BUG_ON(!me->active);
    BUG_ON(me->state != THREAD_AWAKE);
    me->state = THREAD_SLEEPING;

    spin_unlock(&me->lock);
}

static void do_wake(thread_t *t) {
    spin_lock(&t->lock);

    // XXX don't freak out if the thread is already running. For example,
    // we could be asking for a wake from the semaphore code, after the thread
    // has already been asynchronously poked, for example.
    if(t->state == THREAD_SLEEPING) {
        t->state = THREAD_AWAKE;
        if(!t->active) {
            list_add(&t->queue_list, &queued_threads);
        }
    }

    spin_unlock(&t->lock);
}

void thread_wake(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    do_wake(t);

    spin_unlock_irqstore(&sched_lock, flags);
}

void thread_poke(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    if(t->state == THREAD_SLEEPING) {
        do_wake(t);
    }

    spin_unlock_irqstore(&sched_lock, flags);
}

void thread_schedule(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    list_add(&t->list, &threads);
    thread_count++;

    //Pretend it was sleeping
    t->state = THREAD_SLEEPING;
    do_wake(t);

    spin_unlock_irqstore(&sched_lock, flags);
}

//Final thread cleanup will occur in the scheduler running on another stack,
//so that the current one can be freed.
static void thread_die() {
    thread_t *me = current;

    //We're never coming back, so disable interrupts.
    spin_lock_irq(&me->lock);
    me->state = THREAD_EXITED;
    spin_unlock(&me->lock);
    spin_lock(&me->lock);
    spin_unlock(&me->lock);

    sched_switch();
}

static void thread_mark_die(thread_t *t) {
    spin_lock_irq(&t->lock);
    t->should_die = true;
    spin_unlock(&t->lock);
}

void pgroup_send_signal(pgroup_t *pg, uint32_t sig) {
    task_node_t *t;
    LIST_FOR_EACH_ENTRY(t, &pg->members, pgroup_list) {
        task_send_signal(t, sig);
    }
}

void task_send_signal(task_node_t *t, uint32_t sig) {
    //FIXME have returning threads poll this field, and not have to push this
    //on them
    if(list_empty(&t->threads)) {
        return;
    }

    thread_send_signal(list_first(&t->threads, thread_t, thread_list), sig);
}

void thread_send_signal(thread_t *t, uint32_t sig) {
    //FIXME interrupt waiting threads!

    sigaddset(&t->sig_pending, sig);
    thread_poke(t);
}

void task_node_exit(int32_t code) {
    thread_t *me = current;

    task_node_t *node = obtain_task_node(me);
    BUG_ON(!node);
    node->exit_code = code;

    //FIXME don't do this (maybe)
    irqdisable();

    //Only allow one thread to commence the exit process (and aquire the
    //node->lock).
    if(task_node_ensure_not_exiting(node)) {
        //This is not a race because node->lock must be held to update the
        //node's exit state.
        atomic_set(&node->exit_state, TASK_EXITING);

        //Perform un-graceful shootdown of all threads. In the event that it's
        //just us this is graceful, as we just invoke thread_die() when we try
        //to drop back to userland (as the should_die flag is processed then).
        thread_t *t;
        LIST_FOR_EACH_ENTRY(t, &node->threads, thread_list) {
            thread_mark_die(t);
        }

        spin_unlock(&node->lock);
    }
}

void sched_try_resched() {
    check_no_locks_held();

    if(!tasking_up) {
        return;
    }

    thread_t *me = current;

    //This is how threads get removed from circulation
    if(me->should_die) {
        thread_die();
    }

    if(!me || get_percpu(switch_time) <= uptime()
        || me->state != THREAD_AWAKE) {
        sched_switch();
    }
}

static DEFINE_LIST(poll_threads);

static uint32_t suspended_threads = 0;

void sched_suspend_pending_interrupt() {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    thread_t *me = current;

    spin_lock(&me->lock);
    list_add(&me->poll_list, &poll_threads);
    spin_unlock(&me->lock);

    suspended_threads++;

    thread_sleep_prepare();

    spin_unlock_irqstore(&sched_lock, flags);

    sched_switch();
}

void sched_interrupt_notify() {
    check_irqs_disabled();

    spin_lock(&sched_lock);

    thread_t *t;
    LIST_FOR_EACH_ENTRY(t, &poll_threads, poll_list) {
        BUG_ON(t->state != THREAD_SLEEPING);
        suspended_threads--;
        do_wake(t);
    }

    if(suspended_threads) {
        panicf("%u suspended orphans", suspended_threads);
    }

    list_init(&poll_threads);

    spin_unlock(&sched_lock);
}

static bool do_invoke_sigaction(cpu_state_t *state, sig_descriptor_t *sig) {
    thread_t *me = current;

    //Don't deliver most signals to kernel threads
    if(me->flags & THREAD_FLAG_KERNEL
        && !(sig->flags & SD_KERNEL)) {
        return false;
    }

    struct sigaction *sa = &me->node->sigactions[sig->num];

    //Dispatch default handlers, noting that init (pid=1) never catches
    //and signals unless it explicitly installs handlers for them.
    if((sa->sa_handler == SIG_DFL || sig->flags & SD_UNCATCHABLE)
        && me->node->pid != 1) {
        if(sig->handler) {
            sig->handler();
        }
        return true;
    }

    //Is the signal currently masked?
    if(sigismember(&me->node->sig_mask, sig->num)) {
        return false;
    }

    sigdelset(&me->sig_pending, sig->num);

    //Should we ignore the signal?
    if(sa->sa_handler == SIG_IGN) {
        return false;
    }

    //even if SA_SIGINFO is set, the handler pointer is stored in a union...
    void *entry = sa->sa_handler;
    sigset_t old_mask = current->node->sig_mask;
    sigset_t new_mask = old_mask | sa->sa_mask;

    if(sa->sa_flags & SA_ONSTACK) {
        panic("sig - onstack unimplemented");
    }

    if(sa->sa_flags & SA_NOCLDSTOP) {
        panic("sig - nocldstop unimplemented");
    }

    if(sa->sa_flags & SA_SIGINFO) {
        panic("sig - siginfo unimplemented");
    }

    if(sa->sa_flags & SA_RESTART) {
        panic("sig - restart unimplemented");
    }

    // These are linux extensions:
    //
    // if(sa->sa_flags & SA_NOCLDWAIT) {
    //     panic("sig - nocldwait unimplemented");
    // }
    //
    // if(sa->sa_flags & SA_RESETHAND) {
    //     panic("sig - resethand unimplemented");
    // }
    //
    // if(!(sa->sa_flags & (SA_NODEFER | SA_RESETHAND))) {
    sigaddset(&new_mask, sig->num);
    // }

    //It is arch's job to set up the stack with all of the sigreturn data,
    //including setting the return address to the registered sigtramp (but we
    //pass all of the requred data).
    current->node->sig_mask = new_mask;
    arch_setup_sigaction(state, entry, me->node->sigtramp, old_mask);

    return true;
}

void sched_deliver_signals(cpu_state_t *state) {
    check_irqs_disabled();

    if(!tasking_up) {
        return;
    }

    //FIXME LOCK THIS

    thread_t *me = current;
    sigset_t *pending = &me->sig_pending;

    //If there is no registered handler, any signal should immediately be
    //upgraded to SIGKILL --- i.e. right here we immediately invoke
    //thread_die().
    if(*pending && !me->node->sigtramp) {
        sigemptyset(pending);
        sigaddset(pending, SIGKILL);
    }

    for(uint32_t i = 0; i < NSIG; i++) {
        if(sigismember(pending, signals[i].num)
            && do_invoke_sigaction(state, &signals[i])) {
            break;
        }
    }
}

bool are_signals_pending(thread_t *me) {
    return me->sig_pending;
}

bool should_abort_slow_io() {
    return are_signals_pending(current);
}

void thread_destroy(thread_t *t) {
    kfree(t);
}

static volatile uint32_t tasks_going = 0;

//t->lock is must be held
static void deactivate_thread(thread_t *t) {
    if(!t) return;

    switch(t->state) {
        case THREAD_IDLE: {
            break;
        }
        case THREAD_AWAKE: {
            BUG_ON(!t->active);
            list_add(&t->queue_list, &queued_threads);

            FALLTHROUGH;
        }
        case THREAD_SLEEPING: {
            ACCESS_ONCE(tasks_going)--;

            break;
        }
        case THREAD_EXITED: {
            //FIXME this is garbage

            list_rm(&t->list);
            list_rm(&t->thread_list);

            spin_unlock(&t->lock);
            spin_lock(&t->lock);
            spin_unlock(&t->lock);

            put_fs_context(t);
            put_ufds(t);
            put_task_node(t);

            if(list_empty(&t->node->threads)) {
                task_node_zombify(t->node);
            }

            spin_lock(&t->lock);

            //FIXME queue the arch-(i.e. stack) desctruction of this thread
            //processed at next sched iteration. (i.e. not on our stack which
            //will have to be freed)

            break;
        }
        default: {
            BUG();

            break;
        }
    }
}

//lock is already held for old
static inline thread_t * lock_next_current(thread_t *old) {
    thread_t *t;
    while(tasking_up && !list_empty(&queued_threads)) {
        t = list_first(&queued_threads, thread_t, queue_list);
        if(t != old) {
            spin_lock(&t->lock);
        }
        list_rm(&t->queue_list);

        switch(t->state) {
            case THREAD_AWAKE: {
                ACCESS_ONCE(tasks_going)++;
                //found it (lock remains held)
                return t;
            }
            default: {
                panicf("illegal state %u", t->state);
            }
        }
    }

    //We ran out of queued threads.
    thread_t *idle = get_percpu(idle_task);
    if(idle != old) {
        spin_lock(&idle->lock);
    }
    return idle;
}

static void finish_sched_switch(thread_t *old, thread_t *next) {
    check_irqs_disabled();

    current = next;

    BUG_ON(old != next && next->active);
    check_on_correct_stack();

    old->active = false;
    next->active = true;

    BUG_ON(next->state != THREAD_IDLE && next->state != THREAD_AWAKE);

    spin_unlock(&old->lock);
    if(old != next) {
        spin_unlock(&next->lock);
    }
    spin_unlock(&sched_lock);

    get_percpu(switch_time) = uptime() + QUANTUM;

    check_no_locks_held();
    check_irqs_disabled();

    context_switch(next);

    BUG();
}

//IRQs must be disabled
static void __noreturn do_sched_switch() {
    check_irqs_disabled();
    check_no_locks_held();

    spin_lock(&sched_lock);

    thread_t *old = current;
    BUG_ON(!old);

    spin_lock(&old->lock);
    deactivate_thread(old);

    switch_stack(old, lock_next_current(old), finish_sched_switch);

    BUG();
}

void __noreturn sched_loop() {
    irqdisable();

    kprintf("sched - proc #%u is READY", get_percpu(this_proc)->num);

    get_percpu(locks_held) = 0;
    list_init(&get_percpu(lock_list));
    get_percpu(switch_time) = 0;

    thread_t *idle = create_idle_task();
    get_percpu(idle_task) = idle;
    current = idle;

    if(get_percpu(this_proc)->num == BSP_ID) {
        kprintf("sched - activating scheduler");

        percpu_up = true;
        tasking_up = true;
    } else {
        //during init, the APs just fall through to the idle task here, and
        //can never be given a real task until tasking_up = true is set.
    }

    do_sched_switch();

    BUG();
}

static void switch_interrupt(interrupt_t *interrupt, void *data) {
    check_irqs_disabled();

    //We save the stack here and not in do_sched_switch() because sched_loop()
    //invokes do_sched_switch() and we don't care about saving its stack.
    save_stack(&interrupt->cpu);

    barrier();

    eoi_handler(interrupt->vector);

    do_sched_switch();
}

static void sig_ignr() {
    //do nothing
}

static void sig_kill() {
    //FIXME correctly set the exit code (here and in sys__exit())
    task_node_exit(1);
}

static void sig_core() {
    panic("sig - core dump unimplemented");
}

static void sig_stop() {
    panic("sig - stop unimplemented");
}

static void sig_cont() {
    panic("sig - cont unimplemented");
}

static struct sigaction default_sigactions[NSIG] = {
    [SIGHUP  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGINT  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGQUIT ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGILL  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGTRAP ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGABRT ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGEMT  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGFPE  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGKILL ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGBUS  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGSEGV ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGSYS  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGPIPE ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGALRM ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGTERM ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGURG  ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGSTOP ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGTSTP ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGCONT ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGCHLD ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGTTIN ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGTTOU ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGIO   ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGWINCH] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGUSR1 ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
    [SIGUSR2 ] = {.sa_flags = 0, .sa_handler = SIG_DFL},
};

//this order is the signal dispatch order!
static sig_descriptor_t signals[NSIG] = {
    {.num = SIGKILL , .flags = SD_UNCATCHABLE
                             | SD_KERNEL     , .handler = sig_kill},
    {.num = SIGSTOP , .flags = SD_UNCATCHABLE, .handler = sig_stop},
    {.num = SIGHUP  , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGINT  , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGQUIT , .flags = SD_NONE,        .handler = sig_core},
    {.num = SIGILL  , .flags = SD_NONE,        .handler = sig_core},
    {.num = SIGTRAP , .flags = SD_NONE,        .handler = sig_core},
    {.num = SIGABRT , .flags = SD_NONE,        .handler = sig_core},
    {.num = SIGEMT  , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGFPE  , .flags = SD_NONE,        .handler = sig_core},
    {.num = SIGBUS  , .flags = SD_NONE,        .handler = sig_core},
    {.num = SIGSEGV , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGSYS  , .flags = SD_NONE,        .handler = sig_core},
    {.num = SIGPIPE , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGALRM , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGTERM , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGURG  , .flags = SD_NONE,        .handler = sig_ignr},
    {.num = SIGTSTP , .flags = SD_NONE,        .handler = sig_stop},
    {.num = SIGCONT , .flags = SD_NONE,        .handler = sig_cont},
    {.num = SIGCHLD , .flags = SD_NONE,        .handler = sig_ignr},
    {.num = SIGTTIN , .flags = SD_NONE,        .handler = sig_stop},
    {.num = SIGTTOU , .flags = SD_NONE,        .handler = sig_stop},
    {.num = SIGIO   , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGWINCH, .flags = SD_NONE,        .handler = sig_ignr},
    {.num = SIGUSR1 , .flags = SD_NONE,        .handler = sig_kill},
    {.num = SIGUSR2 , .flags = SD_NONE,        .handler = sig_kill},
};

static char *idle_argv[] = {"kidle", NULL};
static char *init_argv[] = {"kinit", NULL};

static INITCALL sched_init() {
    thread_cache = cache_create(sizeof(thread_t));
    idle_node = task_node_build(NULL, idle_argv, NULL);
    init_node = task_node_build(NULL, init_argv, NULL);

    return 0;
}

static INITCALL sched_register() {
    register_isr(SWITCH_INT, CPL_KRNL, switch_interrupt, NULL);

    return 0;
}

core_initcall(sched_init);
subsys_initcall(sched_register);
