#include <stdbool.h>

#include "lib/int.h"
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
#include "sched/task.h"
#include "fs/file/char.h"
#include "log/log.h"
#include "misc/stats.h"

#define MAX_NUM_FDS ((ufd_idx_t) (PAGE_SIZE / sizeof(ufd_t)))

#define FREELIST_END (1 << 31)

#define UFD_FLAG_CLOSING (1 << 0)

#define TASK_FLAG_USER (1 << 0)

static uint32_t pid = 2;

static cache_t *task_cache;

static inline task_t * task_build() {
    task_t *task = cache_alloc(task_cache);
    task->flags = 0;
    task->state = TASK_BUILDING;
    spinlock_init(&task->lock);
    spinlock_init(&task->fd_lock);

    page_t *page = alloc_page(0);
    task->arch.dir = page_to_virt(page);
    task->arch.cr3 = (uint32_t) page_to_phys(page);
    build_page_dir(task->arch.dir);

    task->ufdt = kmalloc(UFDT_PAGES * PAGE_SIZE);
    task->ufd_list = kmalloc(UFD_LIST_PAGES * PAGE_SIZE);
    task->kernel_stack_top = kmalloc(KERNEL_STACK_LEN);
    task->kernel_stack_bottom = task->kernel_stack_top + KERNEL_STACK_LEN;

    return task;
}

void copy_fds(task_t *to, task_t *from) {
    to->fd_next = from->fd_next;

    for(ufd_idx_t i = 0; i < MAX_NUM_FDS - 1; i++) {
        to->ufd_list[i] = from->ufd_list[i];

        to->ufdt[i].gfd = from->ufdt[i].gfd;
        to->ufdt[i].flags = from->ufdt[i].flags;
        to->ufdt[i].refs = from->ufdt[i].refs;
    }
}

void spawn_kernel_task(void (*main)(void *arg), void *arg) {
    task_fork(0, main, arg);
}

void task_fork(uint32_t flags, void (*setup)(void *arg), void *arg) {
    task_count++;

    task_t *child = task_build();
    child->pid = pid++;
    child->argv = current->argv;
    child->envp = current->envp;

    child->root = current->root;
    child->pwd = current->pwd;

    pl_setup_task(child, setup, arg);

    copy_fds(child, current);
    copy_mem(child, current);

    task_add(child);
}

void __init root_task_init(void *umain, char **argv, char **envp) {
    task_count++;

    task_t *init = task_build();
    init->pid = 1;
    init->argv = argv;
    init->envp = envp;

    init->root = ROOT_PATH(root_mount);
    init->pwd = ROOT_PATH(root_mount);

    init->fd_next = 0;

    for(ufd_idx_t i = 0; i < MAX_NUM_FDS - 1; i++) {
        init->ufd_list[i] = i + 1;

        init->ufdt[i].gfd = NULL;
        init->ufdt[i].flags = 0;
        init->ufdt[i].refs = 0;
    }
    init->ufd_list[MAX_NUM_FDS - 1] = FREELIST_END;

    pl_setup_task(init, umain, NULL);

    task_add(init);

    kprintf("task - root task created");
}

static void idle_loop(void *UNUSED(arg)) {
    while(true) hlt();
}

task_t * create_idle_task() {
    task_t *idler = task_build();
    pl_setup_task(idler, idle_loop, NULL);
    idler->state = TASK_IDLE;
    return idler;
}

//FIXME this function is totally broken and basically exits to give a rough
//idea of what the real one should do. never call it.
//TODO implement killing a task by taking control of its EIP and pointing it to
//kernel land
void task_exit(int32_t code) {
    spin_lock(&current->lock);
    current->state = TASK_EXITED;
    spin_unlock(&current->lock);

    list_rm(&current->list);
    list_rm(&current->queue_list);

    task_count--;

    //TODO propagate the exit code somehow

    for(ufd_idx_t i = 0; i < MAX_NUM_FDS; i++) {
        if(ufdt_valid(i)) {
            ufdt_put(i);
        }
    }

    //TODO free task page directory, iterate through it and free all of the non-kernel page tables

    sched_switch();
}

ufd_idx_t ufdt_add(uint32_t flags, file_t *gfd) {
    uint32_t f;
    spin_lock_irqsave(&current->fd_lock, &f);

    ufd_idx_t added = current->fd_next;
    if(added != FREELIST_END) {
        current->fd_next = current->ufd_list[added];

        gfdt_get(gfd);

        current->ufdt[added].refs = 1;
        current->ufdt[added].flags = flags;
        current->ufdt[added].gfd = gfd;
    }

    spin_unlock_irqstore(&current->fd_lock, f);

    return added == FREELIST_END ? -1 : added;
}

bool ufdt_valid(ufd_idx_t ufd) {
    bool response;

    uint32_t flags;
    spin_lock_irqsave(&current->fd_lock, &flags);

    response = !!current->ufdt[ufd].gfd;

    spin_unlock_irqstore(&current->fd_lock, flags);

    return response;
}

//task->fd_lock is required to call this function
static void ufdt_try_free(ufd_idx_t ufd) {
    if(!current->ufdt[ufd].refs) {
        gfdt_put(current->ufdt[ufd].gfd);

        current->ufdt[ufd].gfd = NULL;
        current->ufdt[ufd].flags = 0;
        current->ufd_list[ufd] = current->fd_next;
        current->fd_next = ufd;
    }
}

file_t * ufdt_get(ufd_idx_t ufd) {
    file_t *gfd;

    uint32_t flags;
    spin_lock_irqsave(&current->fd_lock, &flags);

    if(ufd > MAX_NUM_FDS) {
        gfd = NULL;
    } else {
        gfd = current->ufdt[ufd].gfd;

        if(gfd) {
            current->ufdt[ufd].refs++;
        }
    }

    spin_unlock_irqstore(&current->fd_lock, flags);

    return gfd;
}

void ufdt_put(ufd_idx_t ufd) {
    uint32_t flags;
    spin_lock_irqsave(&current->fd_lock, &flags);

    current->ufdt[ufd].refs--;

    ufdt_try_free(ufd);

    spin_unlock_irqstore(&current->fd_lock, flags);
}

void ufdt_close(ufd_idx_t ufd) {
    uint32_t flags;
    spin_lock_irqsave(&current->fd_lock, &flags);

    if(!(current->ufdt[ufd].flags & UFD_FLAG_CLOSING)) {
        current->ufdt[ufd].flags |= UFD_FLAG_CLOSING;
        current->ufdt[ufd].refs--;

        ufdt_try_free(ufd);
    }

    spin_unlock_irqstore(&current->fd_lock, flags);
}

static INITCALL task_init() {
    task_cache = cache_create(sizeof(task_t));

    return 0;
}

core_initcall(task_init);
