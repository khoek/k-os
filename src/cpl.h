#ifndef KERNEL_CPL_H
#define KERNEL_CPL_H

#include <stdint.h>
#include "task.h"

void cpl_switch(task_t *task);

#endif
