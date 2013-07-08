#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include "lib/int.h"

void timer_create(uint32_t millis, void (*callback)(void *), void *data);

#endif
