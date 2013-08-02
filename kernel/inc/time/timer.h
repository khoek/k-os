#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H

#include "lib/int.h"

typedef void (*timer_callback_t)(void *);

void timer_create(uint32_t millis, timer_callback_t callback, void *data);

#endif
