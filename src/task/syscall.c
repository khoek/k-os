#include <stddef.h>

#include "init.h"
#include "syscall.h"
#include "gdt.h"
#include "idt.h"
#include "panic.h"
#include "task.h"
#include "log.h"

#define MAX_SYSCALL 256

typedef void (*syscall_t)(interrupt_t *);

void sys_exit(interrupt_t *interrupt) {
    logf("sys_exit: %d", interrupt->registers.ebx);
    
    task_exit(current, interrupt->registers.ebx);
}

void sys_fork(interrupt_t *interrupt) {
    logf("sys_fork: %d", interrupt->registers.ebx);
}

syscall_t syscalls[MAX_SYSCALL] = {
    [0] = sys_exit,
    [1] = sys_fork
};

static void syscall_handler(interrupt_t *interrupt) {
    if(interrupt->registers.eax >= MAX_SYSCALL || syscalls[interrupt->registers.eax] == NULL) {
        panicf("Unregistered Syscall #%u: 0x%X", interrupt->registers.eax, interrupt->registers.ebx);
    } else {
        sti();
        syscalls[interrupt->registers.eax](interrupt);
    }
}

static INITCALL syscall_init() {
    idt_register(0x80, CPL_USER, syscall_handler);

    return 0;
}

core_initcall(syscall_init);
