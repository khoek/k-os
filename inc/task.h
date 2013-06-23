#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include "int.h"
#include "list.h"
#include "registers.h"

typedef struct task {
    uint32_t pid;
    registers_t registers;
    task_state_t state;
    list_head_t list;
    uint32_t cr3;
    uint32_t *directory;
} task_t;

#include "mm.h"

task_t * task_create();
void task_schedule(task_t *task, void *eip, void *esp);
void task_add_page(task_t *task, page_t *page);
void task_switch();

#endif
