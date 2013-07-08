#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include <stdbool.h>

#include "lib/int.h"
#include "common/compiler.h"
#include "common/init.h"
#include "arch/registers.h"

#define IRQ_OFFSET 0x20

typedef struct interrupt {
    uint32_t vector, error;
    cpu_state_t cpu;
} PACKED interrupt_t;

//indirect, invoked by gdt_init()
INITCALL idt_init();

void idt_register(uint8_t vector, uint8_t cpl, void(*handler)(interrupt_t *));
void idt_set_isr(uint32_t gate, uint32_t isr);
void interrupt_dispatch(interrupt_t * reg);

#endif
