#include <stdbool.h>

#include "int.h"
#include "list.h"
#include "string.h"
#include "init.h"
#include "debug.h"
#include "task.h"
#include "cache.h"
#include "gdt.h"
#include "idt.h"
#include "cpl.h"
#include "mm.h"
#include "panic.h"
#include "clock.h"
#include "asm.h"
#include "log.h"

task_t *current;

static cache_t *task_cache;
static uint32_t pid = 1;
static bool tasking_up = false;

static LIST_HEAD(tasks);
static LIST_HEAD(sleeping_tasks);
static LIST_HEAD(blocking_tasks);

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
    tasking_up = true;

    while(1) hlt();
}

void task_save(cpu_state_t *cpu) {
    if(current) {
        memcpy(&current->cpu.reg, &cpu->reg, sizeof(registers_t));
        memcpy(&current->cpu.exec, &cpu->exec, sizeof(exec_state_t));
    }
}

static void task_switch() {
    if(list_empty(&tasks)) {
        panic("All processes have exited!");
    } else {
        current = list_first(&tasks, task_t, list);
        list_rotate_left(&tasks);
    }

    BUG_ON(current->state != TASK_AWAKE);
}

void task_reschedule() {
    if(!tasking_up) return;

    task_switch();

    cpl_switch(current->cr3, current->cpu.reg, current->cpu.exec);
}

void task_run_scheduler() {
    //TODO descide whether or not to switch tasks

    task_reschedule();
}

void task_add_page(task_t UNUSED(*task), page_t UNUSED(*page)) {
    //TODO add this to the list of pages for the task
}

#define FLAG_KERNEL (1 << 0)

task_t * task_create(bool kernel, void *eip, void *esp) {
    task_t *task = (task_t *) cache_alloc(task_cache);
    task->pid = pid++;

    memset(&task->cpu.reg, 0, sizeof(registers_t));

    task->cpu.exec.esp = (uint32_t) esp;
    task->cpu.exec.eip = (uint32_t) eip;

    task->state = TASK_AWAKE;

    task->cpu.exec.eflags = get_eflags() | EFLAGS_IF;

    if(kernel) {
        task->flags = FLAG_KERNEL;

        task->cpu.exec.cs = SEL_KERNEL_CODE | SPL_KERNEL;
        task->cpu.exec.ss = SEL_KERNEL_DATA | SPL_KERNEL;
    } else {
        task->flags = 0;

        task->cpu.exec.cs = SEL_USER_CODE | SPL_USER;
        task->cpu.exec.ss = SEL_USER_DATA | SPL_USER;
    }

    page_t *page = alloc_page(0);
    task->directory = page_to_virt(page);
    task->cr3 = (uint32_t) page_to_phys(page);
    page_build_directory(task->directory);

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

    register_clock_event_listener(&clock_listener);

    logf("task - set up init task");

    return 0;
}

core_initcall(task_init);
