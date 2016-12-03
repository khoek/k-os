#ifndef KERNEL_ARCH_TSC_H
#define KERNEL_ARCH_TSC_H

#include "common/types.h"
#include "init/initcall.h"
#include "time/clock.h"

extern clock_t tsc_clock;

uint64_t rdtsc();

void __init tsc_init();
void tsc_busywait_100ms();

#endif
