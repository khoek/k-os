#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include "common.h"

void die() NORETURN;

void panic(char* message) NORETURN;
void panicf(char* fmt, ...) NORETURN;

#endif
