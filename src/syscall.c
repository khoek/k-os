#include <stddef.h>
#include "syscall.h"
#include "gdt.h"
#include "idt.h"
#include "panic.h"
#include "console.h"

#define MAX_SYSCALL 256

typedef void (*syscall_t)(interrupt_t *);

void sys_exit(interrupt_t *interrupt) {
    kprintf("sys_exit: %d\n", interrupt->registers.ebx);

    die();
}

void sys_fork(interrupt_t *interrupt) {
    kprintf("sys_fork: %d\n", interrupt->registers.ebx);
}

syscall_t syscalls[MAX_SYSCALL] = {
    [0] = sys_exit,
    [1] = sys_fork
};

static void syscall_handler(interrupt_t *interrupt) {
    if(interrupt->registers.eax >= MAX_SYSCALL || syscalls[interrupt->registers.eax] == NULL) {
        panicf("Unregistered Syscall #%u: 0x%X", interrupt->registers.eax, interrupt->registers.ebx);
    } else {
        syscalls[interrupt->registers.eax](interrupt);
    }
}

void syscall_init() {
    idt_register(0x80, CPL_USER, syscall_handler);
}
