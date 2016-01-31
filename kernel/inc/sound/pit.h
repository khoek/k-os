#ifndef KERNEL_SOUND_PIT_H
#define KERNEL_SOUND_PIT_H

#include "common/types.h"

void play(uint32_t freq);
void stop();
void beep();

#endif
