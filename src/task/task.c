#include "string.h"
#include "init.h"
#include "list.h"
#include "task.h"
#include "cache.h"
#include "gdt.h"
#include "cpl.h"
#include "mm.h"
#include "panic.h"
#include "log.h"

cache_t *task_cache;

uint32_t pid = 1;
uint8_t kernel_stack[0x1000];

LIST_HEAD(tasks);

static void task_usermode() {
    __asm__ volatile("mov $1, %eax");
    __asm__ volatile("mov $8, %ebx");
    __asm__ volatile("int $0x80");

    __asm__ volatile("mov $0, %eax");
    __asm__ volatile("mov $-1, %ebx");
    __asm__ volatile("int $0x80");

    __asm__ volatile("loop:");
    __asm__ volatile("jmp loop");
}

void task_switch() {
    set_kernel_stack((void *) (((uint32_t) kernel_stack) + sizeof(kernel_stack) - 1));
    list_rotate_left(&tasks);

    task_t *task = list_first(&tasks, task_t, list);
    cpl_switch(task->cr3, task->registers, task->state);
}

void task_add_page(task_t UNUSED(*task), page_t UNUSED(*page)) {
    //TODO add this to the list of pages for the task
}

task_t * task_create() {
    task_t *task = (task_t *) cache_alloc(task_cache);
    task->pid = pid++;

    memset(&task->registers, 0, sizeof(registers_t));

    task->state.eflags = get_eflags();
    task->state.cs = CPL_USER_CODE | 3;
    task->state.ss = CPL_USER_DATA | 3;

    page_t *page = alloc_page(0);
    task->directory = page_to_virt(page);
    task->cr3 = (uint32_t) page_to_phys(page);
    page_build_directory(task->directory);

    return task;
}

void task_schedule(task_t *task, void *eip, void *esp) {
    task->state.esp = (uint32_t) esp;
    task->state.eip = (uint32_t) eip;

    list_add(&task->list, &tasks);
}

static INITCALL task_init() {
    task_cache = cache_create(sizeof(task_t));

    task_t *task = task_create();
    memcpy(alloc_page_user(0, task, 0x10000), task_usermode, 0x1000);
    alloc_page_user(0, task, 0x11000);
    task_schedule(task, (void *) 0x10000, (void *) (0x11000 + PAGE_SIZE - 1));

    logf("task - set up init task");

    return 0;
}

core_initcall(task_init);
