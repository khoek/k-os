#include <stddef.h>

#include "string.h"
#include "init.h"
#include "syscall.h"
#include "gdt.h"
#include "idt.h"
#include "panic.h"
#include "asm.h"
#include "task.h"
#include "timer.h"
#include "cache.h"
#include "log.h"

#define MAX_SYSCALL 256

typedef void (*syscall_t)(interrupt_t *);

static void sys_exit(interrupt_t *interrupt) {
    logf("sys_exit: %d", interrupt->cpu.reg.ecx);

    task_exit(current, interrupt->cpu.reg.ecx);
}

static void sys_fork(interrupt_t *interrupt) {
    logf("sys_fork: %d", interrupt->cpu.reg.ecx);
}

static void wake_task(task_t *task) {
    task_wake(task);
}

static void sys_sleep(interrupt_t *interrupt) {
    task_sleep(current);
    timer_create(interrupt->cpu.reg.ecx, (void (*)(void *)) wake_task, current);
    task_reschedule();
}

static void sys_log(interrupt_t *interrupt) {
    if(interrupt->cpu.reg.edx > 1023) {
        logf("syscall - task log string too long!");
    } else {
        char *buff = kmalloc(interrupt->cpu.reg.edx + 1);
        memcpy(buff, (void *) interrupt->cpu.reg.ecx, interrupt->cpu.reg.edx);
        buff[interrupt->cpu.reg.edx] = '\0';

        log(buff);

        kfree(buff, interrupt->cpu.reg.edx + 1);
    }
}

static syscall_t syscalls[MAX_SYSCALL] = {
    [0] = sys_exit,
    [1] = sys_fork,
    [2] = sys_sleep,
    [3] = sys_log
};

static void syscall_handler(interrupt_t *interrupt) {
    if(interrupt->cpu.reg.eax >= MAX_SYSCALL || syscalls[interrupt->cpu.reg.eax] == NULL) {
        panicf("Unregistered Syscall #%u: 0x%X", interrupt->cpu.reg.eax, interrupt->cpu.reg.ecx);
    } else {
        syscalls[interrupt->cpu.reg.eax](interrupt);
    }
}

static INITCALL syscall_init() {
    idt_register(0x80, CPL_USER, syscall_handler);

    return 0;
}

core_initcall(syscall_init);
