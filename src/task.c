#include "string.h"
#include "init.h"
#include "task.h"
#include "cache.h"
#include "gdt.h"
#include "cpl.h"
#include "mm.h"
#include "panic.h"
#include "log.h"

uint32_t pid = 1;
uint8_t kernel_stack[0x1000];
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

void * task_alloc_page(task_t UNUSED(*task), uint32_t UNUSED(vaddr)) {
    page_t *new_page = alloc_page(0);
new_page++;
    //TODO map new_page to address vaddr in task's address space;

    return NULL; //page_to_virt(new_page);
}

task_t * task_create() {
    return cache_alloc(CACHE_TASK); //FIXME do more stuff here
}

void task_usermode() {
    __asm__ volatile("mov $1, %eax");
    __asm__ volatile("mov $8, %ebx");
    __asm__ volatile("int $0x80");

    __asm__ volatile("mov $0, %eax");
    __asm__ volatile("mov $-1, %ebx");
    __asm__ volatile("int $0x80");

    die();
}

static INITCALL task_init() {
    front = back = cache_alloc(CACHE_TASK);
    memset(&front->registers, 0, sizeof(registers_t));

    page_t *stack_page = alloc_page(0);
    page_protect(stack_page, false);

    page_t *code_page = alloc_page(0);
    page_protect(code_page, false);

    memcpy(page_to_virt(code_page), task_usermode, 0x1000);

    front->pid = pid++;
    front->state.cs = CPL_USER_CODE | 3;
    front->state.ss = CPL_USER_DATA | 3;
    front->state.eflags = get_eflags();
    front->state.esp = ((uint32_t) page_to_virt(stack_page)) + PAGE_SIZE - 1;
    front->state.eip = (uint32_t) page_to_virt(code_page);

    logf("task - set up init task");

    return 0;
}

module_initcall(task_init);
