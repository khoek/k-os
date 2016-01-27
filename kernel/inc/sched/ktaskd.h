#ifndef KERNEL_SCHED_KTASKD_H
#define KERNEL_SCHED_KTASKD_H

#include "common/compiler.h"

void ktaskd_init();
void ktaskd_request(void (*main)(void *arg), void *arg);

#endif
