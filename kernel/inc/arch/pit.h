#ifndef KERNEL_ARCH_PIT_H
#define KERNEL_ARCH_PIT_H

#include "init/initcall.h"
#include "time/clock.h"

extern clock_t pit_clock;

void __init pit_init();

void pit_busywait_20ms();

#endif
