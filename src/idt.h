#include <stdint.h>

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

void idt_init();
void idt_register(uint32_t vector, void(*isr)(uint32_t));
void init_isr();
void idt_register_isr(uint32_t gate, uint32_t isr);
uint32_t isr_dispatch(uint32_t interrupt, uint32_t error);
void idt_dispatch();
