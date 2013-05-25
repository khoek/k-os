#ifndef KERNEL_PIT_H
#define KERNEL_PIT_H

#include <stdint.h>

#define TIMER_FREQ 1000

uint64_t uptime();

void play(uint32_t freq);
void stop();
void beep();

void sleep(uint32_t milis);

#endif
