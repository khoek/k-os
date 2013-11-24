#include <stddef.h>

#include "init/initcall.h"
#include "common/compiler.h"
#include "common/asm.h"
#include "common/list.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "arch/gdt.h"
#include "arch/cpl.h"
#include "mm/cache.h"
#include "sched/sched.h"
#include "video/log.h"

#define NUM_VECTORS 256

//command io port of PICs
#define MASTER_COMMAND 0x20
#define SLAVE_COMMAND  0xA0

//data io port of PICs
#define MASTER_DATA 0x21
#define SLAVE_DATA  0xA1

#define PIC_NUM_PINS 8

//interrupt vector offset
#define PIC_MASTER_OFFSET IRQ_OFFSET
#define PIC_SLAVE_OFFSET  (PIC_MASTER_OFFSET + PIC_NUM_PINS)

//PIC commands
#define INIT 0x11 //enter "init" mode
#define EOI  0x20 //end of interrupt handler

//Potential spurious vectors
#define IRQ_SPURIOUS (PIC_NUM_PINS - 1)
#define INT_SPURIOUS_MASTER (PIC_MASTER_OFFSET + IRQ_SPURIOUS)
#define INT_SPURIOUS_SLAVE  (PIC_SLAVE_OFFSET + IRQ_SPURIOUS)

#define PIC_REG_IRR 0x0A
#define PIC_REG_ISR 0x0B

typedef struct idtd {
	uint16_t size;
	uint32_t offset;
} PACKED idtd_t;

typedef struct idt_entry {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t zero;
    uint8_t type;
    uint16_t offset_hi;
} PACKED idt_entry_t;

typedef struct irq_handler {
    isr_t isr;
    void *data;

    list_head_t list;
} irq_handler_t;

static idt_entry_t idt[NUM_VECTORS];
static list_head_t isrs[NUM_VECTORS - IRQ_OFFSET];

void register_isr(uint8_t vector, uint8_t cpl, isr_t handler, void *data) {
    BUG_ON(vector < IRQ_OFFSET);

    irq_handler_t *new = kmalloc(sizeof(irq_handler_t));
    new->isr = handler;
    new->data = data;
    list_add(&new->list, &isrs[vector - IRQ_OFFSET]);

    idt[vector].type |= cpl;
}

void idt_set_isr(uint32_t gate, uint32_t isr) {
    idt[gate].offset_lo = isr & 0xffff;
    idt[gate].offset_hi = (isr >> 16) & 0xffff;
    idt[gate].selector = SEL_KRNL_CODE;
    idt[gate].zero = 0;
    idt[gate].type |= 0x80 /* present */ | 0xe /* 32 bit interrupt gate */;
}

static char* exceptions[32] = {
     [0] =    "Divide-by-zero",
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

/*static uint16_t get_reg(uint16_t pic, uint8_t reg) {
     outb(pic, reg);
     return inb(pic);
}*/

void interrupt_dispatch(interrupt_t *interrupt) {
    if(interrupt->vector < PIC_MASTER_OFFSET) {
        panicf("Exception #%u: %s\nError Code: 0x%X\nEIP: 0x%p", interrupt->vector, exceptions[interrupt->vector] ? exceptions[interrupt->vector] : "Unknown", interrupt->error, interrupt->cpu.exec.eip);
    }

    /*if(interrupt->vector == INT_SPURIOUS_MASTER && !(get_reg(MASTER_COMMAND, PIC_REG_IRR) & (1 << IRQ_SPURIOUS))) {
        return;
    } else if (interrupt->vector == INT_SPURIOUS_SLAVE && !(get_reg(SLAVE_COMMAND, PIC_REG_IRR) & (1 << IRQ_SPURIOUS))) {
        outb(MASTER_COMMAND, EOI);
        return;
    }*/

    task_save(&interrupt->cpu);

    if(!list_empty(&isrs[interrupt->vector - IRQ_OFFSET])) {
        irq_handler_t *handler;
        LIST_FOR_EACH_ENTRY(handler, &isrs[interrupt->vector - IRQ_OFFSET], list) {
            handler->isr(interrupt, handler->data);
        }
    }

    if(interrupt->vector >= PIC_SLAVE_OFFSET) {
          outb(SLAVE_COMMAND, EOI);
    }

    outb(MASTER_COMMAND, EOI);

    sched_try_resched();
}

void idt_init() {
    idtd_t idtd;
    idtd.size = (NUM_VECTORS * sizeof(idt_entry_t)) - 1;
    idtd.offset = (uint32_t) idt;

    lidt(&idtd);
}

extern void register_isr_stubs();

static INITCALL isr_setup() {
    cli();

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

    sti();

    return 0;
}

static INITCALL isr_init() {
    register_isr_stubs();

    for(uint32_t i = 0; i < NUM_VECTORS - IRQ_OFFSET; i++) {
        list_init(&isrs[i]);
    }

    return 0;
}

core_initcall(isr_init);
postarch_initcall(isr_setup);
