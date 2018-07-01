#include <k/syscall.h>

.global _start

.extern main
.extern _exit

.extern _data_start
.extern _data_end

.section .text.entry, "ax"

_start:
    # Push argv, argc, in that order.
    push %ebx
    push %eax

    call main

    push %eax
    call SYSCALL(exit)
