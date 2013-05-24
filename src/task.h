#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stdint.h>
#include "registers.h"

typedef struct task {
    uint32_t pid;
    registers_t registers;
    task_state_t state;
} task_t;

void task_switch();

void task_usermode();

#endif
