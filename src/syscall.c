#include "syscall.h"
#include "idt.h"
#include "panic.h"

static void syscall_handler(interrupt_t *interrupt) {
    panicf("Syscall #%u: %u", interrupt->eax, interrupt->ebx);
}

void syscall_init() {
    idt_register(0x80, syscall_handler);
}
