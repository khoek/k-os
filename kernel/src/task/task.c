#include <stdbool.h>

#include "lib/int.h"
#include "lib/string.h"
#include "common/list.h"
#include "common/init.h"
#include "common/asm.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/cpl.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "task/task.h"
#include "fs/stream/char.h"
#include "video/log.h"

#define FREELIST_END (1 << 31)

task_t *current;

static cache_t *task_cache;
static uint32_t pid = 1;
static bool tasking_up = false;

static task_t *idle_task;

static DEFINE_LIST(tasks);
static DEFINE_LIST(sleeping_tasks);
static DEFINE_LIST(blocking_tasks);

static void idle_loop() {
    while(1) hlt();
}

void task_block(task_t *task) {
    task->state = TASK_BLOCKING;
    list_move_before(&task->list, &blocking_tasks);
}

void task_sleep(task_t *task) {
    task->state = TASK_SLEEPING;
    list_move_before(&task->list, &sleeping_tasks);
}

void task_wake(task_t *task) {
    task->state = TASK_AWAKE;
    list_move_before(&task->list, &tasks);
}

void task_exit(task_t *task, int32_t code) {
    //TODO propagate the exit code somehow

    list_rm(&task->list);

    cache_free(task_cache, task);

    //TODO free task page directory, iterate through it and free all of the non-kernel page tables

    task_reschedule();
}

void task_run() {
    cli();

    tasking_up = true;

    task_reschedule();
}

void task_save(cpu_state_t *cpu) {
    if(current && tasking_up) {
        current->cpu = (uint32_t) cpu;
        current->ret = (((uint64_t) cpu->reg.edx) << 32) | ((uint64_t) cpu->reg.eax);
    }
}

static void task_switch() {
    if(list_empty(&tasks)) {
        current = idle_task;
    } else {
        current = list_first(&tasks, task_t, list);
        list_rotate_left(&tasks);
    }

    BUG_ON(current->state != TASK_AWAKE);
}

void task_reschedule() {
    if(!tasking_up) return;

    task_switch();

    tss_set_stack(current->kernel_stack);

    cpl_switch(current->cr3, (uint32_t) (current->ret & 0xFFFFFFFF), (uint32_t) (current->ret >> 32), current->cpu);
}

void task_run_scheduler() {
    //TODO descide whether or not to switch tasks

    task_reschedule();
}

void task_add_page(task_t UNUSED(*task), page_t UNUSED(*page)) {
    //TODO add this to the list of pages for the task
}

void task_fd_add(task_t *task, uint32_t flags, uint32_t gfd) {
    uint32_t f;
    spin_lock_irqsave(&task->fd_lock, &f);

    task->fd[task->fd_next].flags = flags;
    task->fd[task->fd_next].gfd = gfd;
    task->fd_next = task->fd_list[task->fd_next];

    spin_unlock_irqstore(&task->fd_lock, f);
}

void task_fd_rm(task_t *task, uint32_t fd_idx) {
    uint32_t flags;
    spin_lock_irqsave(&task->fd_lock, &flags);

    task->fd_list[fd_idx] = task->fd_next;
    task->fd_next = fd_idx;

    spin_unlock_irqstore(&task->fd_lock, flags);
}

#define FLAG_KERNEL (1 << 0)

task_t * task_create(bool kernel, void *ip, void *sp) {
    task_t *task = (task_t *) cache_alloc(task_cache);
    task->pid = pid++;
    task->state = TASK_AWAKE;

    task->ret = 0;

    page_t *page = alloc_page(0);
    task->directory = page_to_virt(page);
    task->cr3 = (uint32_t) page_to_phys(page);
    page_build_directory(task->directory);

    task->fd_count = PAGE_SIZE / sizeof(ufd_t);
    task->fd_next = 0;

    uint32_t *tmp_fds_list = (uint32_t *) alloc_page_user(0, task, 0x20000);
    for(uint32_t i = 0; i < task->fd_count - 1; i++) {
        tmp_fds_list[i] = i + 1;
    }
    tmp_fds_list[task->fd_count - 1] = FREELIST_END;

    ufd_t *tmp_fds = (ufd_t *) alloc_page_user(0, task, 0x21000);
    tmp_fds[0].flags = 0;
    tmp_fds[0].gfd = char_stream_alloc();
    tmp_fds[1].flags = 0;
    tmp_fds[1].gfd = char_stream_alloc();
    tmp_fds[2].flags = 0;
    tmp_fds[2].gfd = char_stream_alloc();

    task->fd_list = (void *) 0x20000;
    task->fd = (void *) 0x21000;

    //FIXME the address 0x11000 is hardcoded
    cpu_state_t *cpu = (void *) (((uint32_t) alloc_page_user(0, task, 0x11000)) + PAGE_SIZE - (sizeof(cpu_state_t) + 1));
    task->kernel_stack = 0x11000 + PAGE_SIZE - 1;
    task->cpu = task->kernel_stack - sizeof(cpu_state_t);

    memset(&cpu->reg, 0, sizeof(registers_t));

    cpu->exec.eip = (uint32_t) ip;
    cpu->exec.esp = (uint32_t) sp;

    cpu->exec.eflags = get_eflags() | EFLAGS_IF;

    if(kernel) {
        task->flags = FLAG_KERNEL;

        cpu->exec.cs = SEL_KERNEL_CODE | SPL_KERNEL;
        cpu->exec.ss = SEL_KERNEL_DATA | SPL_KERNEL;
    } else {
        task->flags = 0;

        cpu->exec.cs = SEL_USER_CODE | SPL_USER;
        cpu->exec.ss = SEL_USER_DATA | SPL_USER;
    }

    return task;
}

void task_schedule(task_t *task) {
    list_add_before(&task->list, &tasks);

    if(list_is_singular(&tasks)) task_switch(); //cancel idle task
}

static void time_tick(clock_event_source_t *source) {
    if(tasking_up) task_switch();
}

static clock_event_listener_t clock_listener = {
    .handle = time_tick
};

static INITCALL task_init() {
    task_cache = cache_create(sizeof(task_t));

    idle_task = task_create(true, idle_loop, NULL);

    register_clock_event_listener(&clock_listener);

    logf("task - set up init task");

    return 0;
}

postcore_initcall(task_init);
