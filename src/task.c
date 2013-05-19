#include "task.h"
#include "string.h"
#include "cache.h"
#include "panic.h"
#include "gdt.h"

uint8_t kernel_stack[0x1000];
task_t *front, back;

void task_init() {
    task_t *init = cache_alloc(CACHE_TASK);
    memset(init, 0, sizeof(task_t));

    init->pid = 1;
}

void task_loop() {
    gdt_set_kernel_stack(kernel_stack);
    enter_usermode();
}

void task_usermode() {
    __asm__ volatile("mov $0, %eax");
    __asm__ volatile("mov $1, %ebx");
    __asm__ volatile("int $0x80");
}
