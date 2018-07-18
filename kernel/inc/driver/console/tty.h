#ifndef KERNEL_DRIVER_CONSOLE_TTY_H
#define KERNEL_DRIVER_CONSOLE_TTY_H

#include "fs/vfs.h"
#include "sched/task.h"

void tty_notify();
void tty_create(char *console);

pgroup_t * tty_get_pgroup(file_t *file);
void tty_set_pgroup(file_t *file, pgroup_t *pg);

#endif
