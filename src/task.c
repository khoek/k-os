#include "string.h"
#include "init.h"
#include "task.h"
#include "cache.h"
#include "gdt.h"
#include "cpl.h"
#include "panic.h"
#include "log.h"

uint8_t kernel_stack[0x1000];
uint8_t user_stack[0x1000];
task_t *front, *back;

void task_switch() {
    set_kernel_stack(kernel_stack);

    /*if(front != back) {
        task_t *old = front;
        front = front->next;

        back->next = old;
        old->prev = back;
        old->next = NULL;

        back = old;
    }*/

    cpl_switch(front);
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
    front = back = cache_alloc(CACHE_TASK);
    memset(&front->registers, 0, sizeof(registers_t));

    front->pid = 1;
    front->state.cs = CPL_USER_CODE | 3;
    front->state.ss = CPL_USER_DATA | 3;
    front->state.eflags = get_eflags();
    front->state.esp = (uint32_t) user_stack;
    front->state.eip = (uint32_t) task_usermode;

    logf("task - set up init task");

    return 0;
}

module_initcall(task_init);
