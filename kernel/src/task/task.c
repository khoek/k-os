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
#include "time/clock.h"
#include "task/task.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "video/log.h"

task_t *current;

static cache_t *task_cache;
static uint32_t pid = 1;
static bool tasking_up = false;

static task_t *idle_task;

static LIST_HEAD(tasks);
static LIST_HEAD(sleeping_tasks);
static LIST_HEAD(blocking_tasks);

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

core_initcall(task_init);
