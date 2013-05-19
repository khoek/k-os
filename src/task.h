#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stdint.h>
#include "registers.h"

typedef struct task {
    uint32_t pid;
    registers_t registers;
    state_t state;
} task_t;

void task_usermode();

void task_init();
void task_start();

#endif
