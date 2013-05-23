#include "string.h"
#include "init.h"
#include "task.h"
#include "cache.h"
#include "gdt.h"
#include "panic.h"
#include "log.h"

uint8_t kernel_stack[0x1000];
task_t *front, back;

void task_start() {
    set_kernel_stack(kernel_stack);
    enter_usermode();
}

void task_usermode() {
    __asm__ volatile("mov $1, %eax");
    __asm__ volatile("mov $8, %ebx");
    __asm__ volatile("int $0x80");

    __asm__ volatile("mov $0, %eax");
    __asm__ volatile("mov $-1, %ebx");
    __asm__ volatile("int $0x80");
}

static INITCALL task_init() {
    task_t *init = cache_alloc(CACHE_TASK);
    memset(init, 0, sizeof(task_t));

    init->pid = 1;

    logf("task - setup init task");

    return 0;
}

module_initcall(task_init);
