#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <stdbool.h>

#include "common/compiler.h"
#include "common/asm.h"

static inline void die() NORETURN;

static inline void die() {
    while(true) hlt();
}

void panic(char* message) NORETURN;
void panicf(char* fmt, ...) NORETURN;

#endif
