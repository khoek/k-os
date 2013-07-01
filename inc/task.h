#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include "int.h"
#include "list.h"
#include "registers.h"
#include "idt.h"

typedef enum task_state {
    TASK_NONE,
    TASK_AWAKE,
    TASK_SLEEPING,
    TASK_BLOCKING
} task_state_t;

typedef struct task {
    list_head_t list;

    registers_t registers;
    proc_state_t proc;
    uint32_t cr3;
    uint32_t *directory;

    uint32_t pid;
    uint32_t flags;
    task_state_t state;
} task_t;

#include "mm.h"

extern task_t *current;

task_t * task_create(bool kernel, void *eip, void *esp);
void task_schedule(task_t *task);
void task_add_page(task_t *task, page_t *page);

void task_exit(task_t *task, int32_t code);

void task_block(task_t *task);
void task_sleep(task_t *task);
void task_wake(task_t *task);

void task_save(interrupt_t *interrupt);
void task_switch();
void task_run();

#endif
