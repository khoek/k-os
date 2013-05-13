#include <stddef.h>
#include "idt.h"
#include "io.h"
#include "panic.h"
#include "console.h"

//command io port of PICs
#define MASTER_COMMAND    0x20
#define SLAVE_COMMAND    0xA0

//data io port of PICs
#define MASTER_DATA    0x21
#define SLAVE_DATA    0xA1

//interrupt vector offset
#define PIC_MASTER_OFFSET    0x20
#define PIC_SLAVE_OFFSET    0x28

//PIC commands
#define INIT    0x11 //enter "init" mode
#define EOI    0x20 //end of interrupt handler

//Potential spurious vectors
#define IRQ_SPURIOUS        7
#define INT_SPURIOUS_MASTER    (PIC_MASTER_OFFSET + IRQ_SPURIOUS)
#define INT_SPURIOUS_SLAVE    (PIC_SLAVE_OFFSET + IRQ_SPURIOUS)

#define PIC_REG_IRR    0x0A
#define PIC_REG_ISR    0x0B

static idtr_t idtr;
static idt_entry_t* idt = (idt_entry_t *) 0x70000;
static void (*isrs[255 - PIC_MASTER_OFFSET])(uint32_t);

void idt_register(uint32_t vector, void(*isr)(uint32_t)) {
    if (isrs[vector - PIC_MASTER_OFFSET]) {
        panicf("Registering second interrupt handler for %u", vector);
    } else {
        isrs[vector - PIC_MASTER_OFFSET] = isr;
    }
}

void idt_set_gate(uint8_t gate, idt_entry_t entry) {
    idt[gate] = entry;
}

void idt_register_isr(uint32_t gate, uint32_t isr) {
    idt[gate].offset_lo = isr & 0xffff;
    idt[gate].offset_hi = (isr >> 16) & 0xffff;
    idt[gate].cs = 0x08;
    idt[gate].zero = 0;
    idt[gate].type = 0x80 /* present */ | 0xe /* 32 bit interrupt gate */;
}

static char* exceptions[32] = {
     [0] =    "Divide-by-zero Error",
     [1] =    "Debug",
     [2] =    "Non-maskable Interrupt",
     [3] =    "Breakpoint",
     [4] =    "Overflow",
     [5] =    "Bound Range Exceeded",
     [6] =    "Invalid Opcode",
     [7] =    "Device Not Available",
     [8] =    "Double Fault",
     [9] =    "Coprocessor Segment Overrun",
     [10] =    "Invalid TSS",
     [11] =    "Segment Not Present",
     [12] =    "Stack-Segment Fault",
     [13] =    "General Protection Fault",
     [14] =    "Page Fault",
     [16] =    "x87 Floating-Point Exception",
     [17] =    "Alignment Check",
     [18] =    "Machine Check",
     [19] =    "SIMD Floating-Point Exception",
     [30] =    "Security Exception"
};

static uint16_t get_reg(uint16_t pic, uint8_t reg) {
     outb(pic, reg);
     return inb(pic);
}

uint32_t isr_dispatch(uint32_t vector, uint32_t error) {
    if (vector < PIC_MASTER_OFFSET) {
        panicf("Exception: Code %u %s - 0x%X", vector, exceptions[vector] ? exceptions[vector] : "Unknown", error);
    }

    get_reg(MASTER_COMMAND, PIC_REG_ISR);

    if(vector == INT_SPURIOUS_MASTER && !(get_reg(MASTER_COMMAND, PIC_REG_ISR) & (1 << IRQ_SPURIOUS))) {
      return 0;
    } else if (vector == INT_SPURIOUS_SLAVE && !(get_reg(SLAVE_COMMAND, PIC_REG_ISR) & (1 << IRQ_SPURIOUS))) {
      outb(MASTER_COMMAND, EOI);
      return 0;
    }

    if (isrs[vector - PIC_MASTER_OFFSET] == NULL) {
        panicf("no registered handler for interrupt %u", vector);
    } else {
        isrs[vector - PIC_MASTER_OFFSET](error);
    }

    if (vector >= PIC_SLAVE_OFFSET) {
          outb(SLAVE_COMMAND, EOI);
    }

    outb(MASTER_COMMAND, EOI);

    return 0;
}

void idt_init() {     
    cli();
     
    init_isr();

    //send INIT command
    outb(MASTER_COMMAND, INIT);
    outb(SLAVE_COMMAND , INIT);

    //set interrupt vector offset
    outb(MASTER_DATA, PIC_MASTER_OFFSET);
    outb(SLAVE_DATA , PIC_SLAVE_OFFSET);

    //set master/slave status
    outb(MASTER_DATA, 2);
    outb(SLAVE_DATA , 4);

    //set mode
    outb(MASTER_DATA, 0x05);
    outb(SLAVE_DATA , 0x01);

    //clear IRQ masks
    outb(MASTER_DATA, 0x0);
    outb(SLAVE_DATA , 0x0);

    idtr.size = (256 * 8) - 1;
    idtr.offset = (uint32_t) idt;

    lidt(&idtr);

    sti();
}
