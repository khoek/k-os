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

typedef struct idtr {
	uint16_t size;
	uint32_t offset;
} __attribute__((__packed__)) idtr_t;

typedef struct idt_entry {
    uint16_t offset_lo;
    uint16_t cs;
    uint8_t zero;
    uint8_t type;
    uint16_t offset_hi;
} __attribute__((__packed__)) idt_entry_t;

void idt_init();

void idt_register(uint32_t vector, void(*isr)(uint32_t));
void init_isr();
void idt_register_handler(uint8_t gate, uint32_t isr);
uint32_t isr_dispatch(uint32_t interrupt, uint32_t error);
void idt_dispatch();
