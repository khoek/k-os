#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include <stdint.h>
#include <stdbool.h>
#include "common.h"
#include "init.h"
#include "registers.h"

static inline void cli() {
    __asm__ volatile("cli");
}

static inline void sti() {
    __asm__ volatile("sti");
}

static inline void hlt() {
    __asm__ volatile("hlt");
}

static inline void lidt(void *idtr) {
   __asm__ volatile("lidt (%0)" :: "p" (idtr));
}

typedef struct interrupt {
  registers_t registers;
  uint32_t vector, error;
  task_state_t state;
} PACKED interrupt_t;

//indirect, invoked by gdt_init()
INITCALL idt_init();

void idt_register(uint8_t vector, uint8_t cpl, void(*handler)(interrupt_t *));
void idt_set_isr(uint32_t gate, uint32_t isr);
void interrupt_dispatch(interrupt_t * reg);

#endif
