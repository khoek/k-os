#ifndef KERNEL_BUG_PANIC_H
#define KERNEL_BUG_PANIC_H

#include <stdbool.h>
#include "common/compiler.h"
#include "common/asm.h"

#define die() do { while(true) hlt(); } while(0)

void __noreturn panic(char* message);
void __noreturn panicf(char* fmt, ...);

#endif
