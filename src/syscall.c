#include <stddef.h>
#include "syscall.h"
#include "idt.h"
#include "panic.h"

#define MAX_SYSCALL 256

typedef void (*syscall_t)(interrupt_t *);

void sys_exit(interrupt_t *interrupt) {
    panicf("sys_exit: %d", interrupt->ebx);
}

void sys_fork(interrupt_t *interrupt) {
    panicf("sys_fork: %d", interrupt->ebx);
}

syscall_t syscalls[MAX_SYSCALL] = {
    [0] = sys_exit,
    [1] = sys_fork
};

static void syscall_handler(interrupt_t *interrupt) {
    if(interrupt->eax >= MAX_SYSCALL || syscalls[interrupt->eax] == NULL) {
        panicf("Unregistered Syscall #%u: 0x%X", interrupt->eax, interrupt->ebx);
    } else {
        syscalls[interrupt->eax](interrupt);
    }
}

void syscall_init() {
    idt_register(0x80, syscall_handler);
}

