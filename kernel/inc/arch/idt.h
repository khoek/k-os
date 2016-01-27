#ifndef KERNEL_ARCH_IDT_H
#define KERNEL_ARCH_IDT_H

#include <stdbool.h>

#define IRQ_OFFSET 0x20

#include "lib/int.h"
#include "common/compiler.h"
#include "init/initcall.h"
#include "arch/pic.h"
#include "arch/cpu.h"

typedef struct interrupt {
    uint32_t vector, error;
    cpu_state_t cpu;
} PACKED interrupt_t;

typedef void (*isr_t)(interrupt_t *interrupt, void *data);

extern void (*eoi_handler)(uint32_t vector);
extern bool (*is_spurious)(uint32_t vector);

void idt_init();

void register_isr(uint8_t vector, uint8_t cpl, void (*handler)(interrupt_t *interrupt, void *data), void *data);
void idt_set_isr(uint32_t gate, uint32_t isr);
void interrupt_dispatch(interrupt_t * reg);

#endif
