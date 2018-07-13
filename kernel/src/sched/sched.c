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

#define MAX_NUM_FDS ((ufd_idx_t) (PAGE_SIZE / sizeof(ufd_t)))

#define FREELIST_END ((uint32_t) (1 << 31))

#define UFD_FLAG_CLOSING (1 << 0)

#define TASK_FLAG_USER (1 << 0)

static pid_t pid = 0;

static cache_t *thread_cache;
static task_node_t *init_node;
static task_node_t *idle_node;

volatile bool tasking_up = false;

static DEFINE_LIST(threads);
static DEFINE_LIST(queued_threads);

//Must be aquired before any task locks, used when manipulating the queue.
//Must be aquired before the heirarchy lock.
static DEFINE_SPINLOCK(sched_lock);

static DEFINE_PER_CPU(thread_t *, idle_task);
static DEFINE_PER_CPU(uint64_t, switch_time);

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
    ufd->list = kmalloc(UFD_LIST_PAGES * PAGE_SIZE);
    ufd->next = 0;
    spinlock_init(&ufd->lock);

    for(ufd_idx_t i = 0; i < MAX_NUM_FDS - 1; i++) {
        ufd->list[i] = i + 1;

        ufd->table[i].gfd = NULL;
        ufd->table[i].flags = 0;
        ufd->table[i].refs = 0;
    }
    ufd->list[MAX_NUM_FDS - 1] = FREELIST_END;

    return ufd;
}

static inline void ufd_context_destroy(ufd_context_t *ufd) {
    for(uint32_t i = 0; i < MAX_NUM_FDS; i++) {
        if(ufd->table[i].gfd) {
            gfdt_put(ufd->table[i].gfd);
        }
    }

    kfree(ufd->table);
    kfree(ufd->list);
    kfree(ufd);
}

static inline ufd_context_t * ufd_context_dup(ufd_context_t *src) {
    ufd_context_t *dst = kmalloc(sizeof(ufd_context_t));
    dst->refs = 1;
    dst->table = kmalloc(UFDT_PAGES * PAGE_SIZE);
    dst->list = kmalloc(UFD_LIST_PAGES * PAGE_SIZE);
    spinlock_init(&dst->lock);

    uint32_t flags;
    spin_lock_irqsave(&src->lock, &flags);

    dst->next = src->next;
    for(ufd_idx_t i = 0; i < MAX_NUM_FDS - 1; i++) {
        dst->list[i] = src->list[i];

        if(src->table[i].gfd) gfdt_get(src->table[i].gfd);

        dst->table[i].gfd = src->table[i].gfd;
        dst->table[i].flags = src->table[i].flags;
        dst->table[i].refs = src->table[i].refs;
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

    if(parent) {
        uint32_t flags;
        spin_lock_irqsave(&parent->lock, &flags);

        list_add(&node->child_list, &parent->children);

        spin_unlock_irqstore(&parent->lock, flags);
    }

    list_init(&node->threads);
    list_init(&node->children);
    list_init(&node->zombies);

    return node;
}

static inline void task_node_destroy(task_node_t *node) {
    kfree(node);
}

//Invoked under node->lock.
static inline void task_node_zombify(task_node_t *node) {
    //TODO when reaped invoke destroy

    atomic_set(&node->exit_state, TASK_EXITED);

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
        if(!node->refs) {
            task_node_zombify(node);
        }
        spin_unlock_irqstore(&node->lock, flags);
    }
}

static inline thread_t * thread_build(task_node_t *node, ufd_context_t *ufd,
        fs_context_t *fs) {
    thread_t *thread = cache_alloc(thread_cache);
    thread->state = THREAD_BUILDING;
    thread->sig_pending = 0;
    thread->node = node;
    thread->ufd = ufd;
    thread->fs = fs;
    thread->flags = 0;
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

ufd_idx_t ufdt_add(uint32_t flags, file_t *gfd) {
    ufd_context_t *ufds = obtain_ufds(current);

    uint32_t f;
    spin_lock_irqsave(&ufds->lock, &f);

    ufd_idx_t added = ufds->next;
    if(added != FREELIST_END) {
        ufds->next = ufds->list[added];

        gfdt_get(gfd);

        ufds->table[added].refs = 1;
        ufds->table[added].flags = flags;
        ufds->table[added].gfd = gfd;
    }

    spin_unlock_irqstore(&ufds->lock, f);

    return added == FREELIST_END ? (uint32_t) -1 : added;
}

bool ufdt_valid(ufd_idx_t ufd) {
    ufd_context_t *ufds = obtain_ufds(current);

    bool response;

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    response = !!ufds->table[ufd].gfd;

    spin_unlock_irqstore(&ufds->lock, flags);

    return response;
}

//thread->fd_lock is required to call this function
static void ufdt_try_free(ufd_idx_t ufd) {
    ufd_context_t *ufds = obtain_ufds(current);

    if(!ufds->table[ufd].refs) {
        gfdt_put(ufds->table[ufd].gfd);

        ufds->table[ufd].gfd = NULL;
        ufds->table[ufd].flags = 0;
        ufds->list[ufd] = ufds->next;
        ufds->next = ufd;
    }
}

file_t * ufdt_get(ufd_idx_t ufd) {
    ufd_context_t *ufds = obtain_ufds(current);

    file_t *gfd;

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    if(ufd > MAX_NUM_FDS) {
        gfd = NULL;
    } else {
        gfd = ufds->table[ufd].gfd;

        if(gfd) {
            ufds->table[ufd].refs++;
        }
    }

    spin_unlock_irqstore(&ufds->lock, flags);

    return gfd;
}

void ufdt_put(ufd_idx_t ufd) {
    ufd_context_t *ufds = obtain_ufds(current);

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    ufds->table[ufd].refs--;

    ufdt_try_free(ufd);

    spin_unlock_irqstore(&ufds->lock, flags);
}

void ufdt_close(ufd_idx_t ufd) {
    ufd_context_t *ufds = obtain_ufds(current);

    uint32_t flags;
    spin_lock_irqsave(&ufds->lock, &flags);

    if(!(ufds->table[ufd].flags & UFD_FLAG_CLOSING)) {
        ufds->table[ufd].flags |= UFD_FLAG_CLOSING;
        ufds->table[ufd].refs--;

        ufdt_try_free(ufd);
    }

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

    BUG_ON(t->state != THREAD_SLEEPING);

    t->state = THREAD_AWAKE;
    if(!t->active) {
        list_add(&t->state_list, &queued_threads);
    }

    spin_unlock(&t->lock);
}

void thread_wake(thread_t *t) {
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, &flags);

    do_wake(t);

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
void thread_exit() {
    thread_t *me = current;

    //We're never coming back, so disable interrupts.
    spin_lock_irq(&me->lock);
    me->state = THREAD_EXITED;
    spin_unlock(&me->lock);
    spin_lock(&me->lock);
    spin_unlock(&me->lock);

    sched_switch();
}

#define S_DIE 0

void thread_send_signal(thread_t *t, uint32_t sig) {
    set_bit(&t->sig_pending, sig);
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
        //just us this is graceful, as we just invoke thread_exit() when we try
        //to drop back to userland (as the S_DIE is processed then).
        thread_t *t;
        LIST_FOR_EACH_ENTRY(t, &node->threads, thread_list) {
            thread_send_signal(t, S_DIE);
        }

        spin_unlock(&node->lock);
    }
}

void sched_try_resched() {
    check_no_locks_held();

    if(tasking_up && (!current || get_percpu(switch_time) <= uptime()
        || current->state != TASK_RUNNING)) {
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

void deliver_signals() {
    check_irqs_disabled();

    //FIXME implement this

    thread_t *me = current;

    if(test_bit(&me->sig_pending, S_DIE)) {
        thread_exit();
        BUG();
    }
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
            list_add(&t->state_list, &queued_threads);

            FALLTHROUGH;
        }
        case THREAD_SLEEPING: {
            ACCESS_ONCE(tasks_going)--;

            break;
        }
        case THREAD_EXITED: {
            //FIXME this is garbage

            list_rm(&t->list);

            spin_unlock(&t->lock);
            spin_lock(&t->lock);
            spin_unlock(&t->lock);

            put_fs_context(t);
            put_ufds(t);
            put_task_node(t);

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
    while(!list_empty(&queued_threads)) {
        t = list_first(&queued_threads, thread_t, state_list);
        if(t != old) {
            spin_lock(&t->lock);
        }
        list_rm(&t->state_list);

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
        //FIXME don't busy wait
        while(!tasking_up);
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
