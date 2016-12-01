#ifndef KERNEL_SCHED_KTASKD_H
#define KERNEL_SCHED_KTASKD_H

void ktaskd_init();
void ktaskd_request(char *name, void (*main)(void *arg), void *arg);

#endif
