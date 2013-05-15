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

typedef struct interrupt {
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t vector, error;
  uint32_t eip, cs, eflags, oldesp, ss;
} interrupt_t;

void idt_init();
void idt_register(uint32_t vector, void(*handler)(interrupt_t *));
void idt_set_isr(uint32_t gate, uint32_t isr);
void interrupt_dispatch(interrupt_t * reg);
