#ifndef KERNEL_ARCH_INTERRUPT_H
#define KERNEL_ARCH_INTERRUPT_H

#include "arch/interrupt.h"

#define EFLAGS_IF (1 << 9)

//These are the only flags which may be freely modified by user mode processes.
#define EFLAGS_USER_MASK 0x8CFF

uint32_t get_eflags();
void set_eflags(uint32_t flags);

static bool are_interrupts_enabled() {
    return get_eflags() & EFLAGS_IF;
}

static void irqdisable() {
    cli();
}

static void irqenable() {
    sti();
}

static void irqsave(uint32_t *flags) {
    *flags = get_eflags() & EFLAGS_IF;
    irqdisable();
}

static void irqstore(uint32_t flags) {
    if(flags & EFLAGS_IF) irqenable();
    else irqdisable();
}

#endif
