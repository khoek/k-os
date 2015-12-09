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
#include "arch/cpl.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "sched/task.h"
#include "fs/file/char.h"
#include "video/log.h"
#include "misc/stats.h"

#define UFD_FLAG_PRESENT (1 << 0)

#define FREELIST_END (1 << 31)

#define FLAG_KERNEL (1 << 0)

static uint32_t pid = 1;

static cache_t *task_cache;

task_t * task_create(const char *name, bool kernel, void *ip, void *sp) {
    task_count++;

    task_t *task = (task_t *) cache_alloc(task_cache);

    task->name = name;
    task->pid = pid++;
    task->state = TASK_AWAKE;

    spinlock_init(&task->lock);

    task->ret = 0;

    task->root.mount = root_mount;
    task->root.dentry = root_mount ? root_mount->fs->root : NULL;

    task->pwd.mount = root_mount;
    task->pwd.dentry = root_mount ? root_mount->fs->root : NULL;

    spinlock_init(&task->fd_lock);

    page_t *page = alloc_page(0);
    task->directory = page_to_virt(page);
    task->cr3 = (uint32_t) page_to_phys(page);
    page_build_directory(task->directory);

    task->fd_count = PAGE_SIZE / sizeof(ufd_t);
    task->fd_next = 3;

    uint32_t *tmp_fds_list = (uint32_t *) alloc_page_user(0, task, 0x20000);
    for(ufd_idx_t i = 3; i < task->fd_count - 1; i++) {
        tmp_fds_list[i] = i + 1;
    }
    tmp_fds_list[task->fd_count - 1] = FREELIST_END;

    ufd_t *tmp_fds = (ufd_t *) alloc_page_user(0, task, 0x21000);
    tmp_fds[0].flags = UFD_FLAG_PRESENT;
    tmp_fds[0].gfd = char_file_open(512);
    tmp_fds[0].refs = 1;
    tmp_fds[1].flags = UFD_FLAG_PRESENT;
    tmp_fds[1].gfd = char_file_open(512);
    tmp_fds[1].refs = 1;
    tmp_fds[2].flags = UFD_FLAG_PRESENT;
    tmp_fds[2].gfd = char_file_open(512);
    tmp_fds[2].refs = 1;

    for(ufd_idx_t i = 3; i < task->fd_count - 1; i++) {
        tmp_fds[i].gfd = FD_INVALID;
        tmp_fds[i].flags = 0;
        tmp_fds[i].refs = 0;
    }

    task->fd_list = (void *) 0x20000;
    task->fd = (void *) 0x21000;

    //FIXME the address 0x11000 is hardcoded
    cpu_state_t *cpu = (void *) (((uint32_t) alloc_page_user(0, task, 0x11000)) + PAGE_SIZE - (sizeof(cpu_state_t)));
    task->kernel_stack = 0x11000 + PAGE_SIZE;
    task->cpu = task->kernel_stack - sizeof(cpu_state_t);

    memset(&cpu->reg, 0, sizeof(registers_t));

    cpu->exec.eip = (uint32_t) ip;
    cpu->exec.esp = (uint32_t) sp;

    cpu->exec.eflags = get_eflags() | EFLAGS_IF;

    if(kernel) {
        task->flags = FLAG_KERNEL;

        cpu->exec.cs = SEL_KRNL_CODE | SPL_KRNL;
        cpu->exec.ss = SEL_KRNL_DATA | SPL_KRNL;
    } else {
        task->flags = 0;

        cpu->exec.cs = SEL_USER_CODE | SPL_USER;
        cpu->exec.ss = SEL_USER_DATA | SPL_USER;
    }

    return task;
}

void task_add_page(task_t UNUSED(*task), page_t UNUSED(*page)) {
    //TODO add this to the list of pages for the task
}

void task_destroy(task_t *task) {
    cache_free(task_cache, task);
}

ufd_idx_t ufdt_add(task_t *task, uint32_t flags, gfd_idx_t gfd) {
    uint32_t f;
    spin_lock_irqsave(&task->fd_lock, &f);

    ufd_idx_t added = task->fd_next;
    task->fd[task->fd_next].flags = UFD_FLAG_PRESENT | flags;
    task->fd[task->fd_next].refs = 1;
    task->fd[task->fd_next].gfd = gfd;
    task->fd_next = task->fd_list[task->fd_next];

    spin_unlock_irqstore(&task->fd_lock, f);

    return added;
}

bool ufdt_valid(task_t *task, ufd_idx_t ufd) {
    bool response;

    uint32_t flags;
    spin_lock_irqsave(&task->fd_lock, &flags);

    response = task->fd[ufd].gfd != FD_INVALID;

    spin_unlock_irqstore(&task->fd_lock, flags);

    return response;
}

gfd_idx_t ufdt_get(task_t *task, ufd_idx_t ufd) {
    gfd_idx_t gfd;

    uint32_t flags;
    spin_lock_irqsave(&task->fd_lock, &flags);

    if(ufd > task->fd_count) {
        gfd = FD_INVALID;
    } else {
        gfd = task->fd[ufd].gfd;

        if(gfd != FD_INVALID) {
            task->fd[ufd].refs++;
        }
    }

    spin_unlock_irqstore(&task->fd_lock, flags);

    return gfd;
}

void ufdt_put(task_t *task, ufd_idx_t ufd) {
    uint32_t flags;
    spin_lock_irqsave(&task->fd_lock, &flags);

    BUG_ON(task->fd[ufd].gfd == FD_INVALID);

    task->fd[ufd].refs--;

    if(!task->fd[ufd].refs) {
        gfdt_put(task->fd[ufd].gfd);

        task->fd[ufd].gfd = FD_INVALID;
        task->fd[ufd].flags = 0;
        task->fd_list[ufd] = task->fd_next;
        task->fd_next = ufd;
    }

    spin_unlock_irqstore(&task->fd_lock, flags);
}

static INITCALL task_init() {
    task_cache = cache_create(sizeof(task_t));

    return 0;
}

core_initcall(task_init);
