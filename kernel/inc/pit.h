#ifndef KERNEL_PIT_H
#define KERNEL_PIT_H

#include "int.h"

void play(uint32_t freq);
void stop();
void beep();

#endif
