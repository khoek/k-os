#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include "int.h"

void timer_create(uint32_t millis, void (*callback)(void *), void *data);

#endif
