#include "init/initcall.h"
#include "common/compiler.h"
#include "common/asm.h"
#include "common/list.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "arch/gdt.h"
#include "arch/pl.h"
#include "mm/cache.h"
#include "sched/sched.h"
#include "log/log.h"

#define NUM_VECTORS 256

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

void (*eoi_handler)(uint32_t vector);
bool (*is_spurious)(uint32_t vector);

static idt_entry_t idt[NUM_VECTORS];
static list_head_t isrs[NUM_VECTORS - IRQ_OFFSET];

void register_isr(uint8_t vector, uint8_t cpl, isr_t handler, void *data) {
    BUG_ON(vector < IRQ_OFFSET);

    irq_handler_t *new = kmalloc(sizeof(irq_handler_t));
    new->isr = handler;
    new->data = data;
    list_add(&new->list, &isrs[vector - IRQ_OFFSET]);

	//FIXME this doesn't make sense if multiple handlers are allowed
    idt[vector].type |= cpl;
}

void idt_set_isr(uint32_t gate, uint32_t isr) {
    idt[gate].offset_lo = isr & 0xffff;
    idt[gate].offset_hi = (isr >> 16) & 0xffff;
    idt[gate].selector = SEL_KRNL_CODE;
    idt[gate].zero = 0;
    idt[gate].type |= 0x80 /* present */ | 0xe /* 32 bit interrupt gate */;
}

#define EX_PAGE_FAULT 14

static char* exceptions[32] = {
     [ 0] = "Divide-by-zero",
     [ 1] = "Debug",
     [ 2] = "Non-maskable Interrupt",
     [ 3] = "Breakpoint",
     [ 4] = "Overflow",
     [ 5] = "Bound Range Exceeded",
     [ 6] = "Invalid Opcode",
     [ 7] = "Device Not Available",
     [ 8] = "Double Fault",
     [ 9] = "Coprocessor Segment Overrun",
     [10] = "Invalid TSS",
     [11] = "Segment Not Present",
     [12] = "Stack-Segment Fault",
     [13] = "General Protection Fault",
     [EX_PAGE_FAULT] = "Page Fault",
     [16] = "x87 Floating-Point Exception",
     [17] = "Alignment Check",
     [18] = "Machine Check",
     [19] = "SIMD Floating-Point Exception",
     [30] = "Security Exception"
};

static void handle_exception(interrupt_t *interrupt) {
    if(panic_in_progress) {
        die();
    }

	switch(interrupt->vector) {
		case EX_PAGE_FAULT: {
			uint32_t cr2;
			__asm__ __volatile__ ("mov %%cr2, %0" : "=r" (cr2));
			panicf("Exception #%u: %s\nError Code: 0x%X\nCR2: 0x%X\nEIP: 0x%p",
				EX_PAGE_FAULT, "Page Fault", cr2,
				interrupt->error, interrupt->cpu.exec.eip);
				break;
		}
		default: {
			panicf("Exception #%u: %s\nError Code: 0x%X\nEIP: 0x%p",
				interrupt->vector,
				exceptions[interrupt->vector] ? exceptions[interrupt->vector] : "Unknown",
				interrupt->error, interrupt->cpu.exec.eip);
			break;
		}
	}
}

void interrupt_dispatch(interrupt_t *interrupt) {
    check_on_correct_stack();
		check_irqs_disabled();

    if(interrupt->vector < IRQ_OFFSET) {
        handle_exception(interrupt);
    }

		if(!is_spurious(interrupt->vector)
				&& !list_empty(&isrs[interrupt->vector - IRQ_OFFSET])) {
        irq_handler_t *handler;
        LIST_FOR_EACH_ENTRY(handler, &isrs[interrupt->vector - IRQ_OFFSET], list) {
            handler->isr(interrupt, handler->data);
						check_irqs_disabled();
        }
    }

		sched_interrupt_notify();

    eoi_handler(interrupt->vector);

    sched_try_resched();

    //This might not return (as in SIGKILL).
    //FIXME call deliver_signals() after every interrupt where we are returning
    //to a user task.
    sched_deliver_signals(&interrupt->cpu);
}

void idt_init() {
    volatile idtd_t idtd;
    idtd.size = (NUM_VECTORS * sizeof(idt_entry_t)) - 1;
    idtd.offset = (uint32_t) idt;

    lidt(&idtd);
}

extern void register_isr_stubs();

static INITCALL isr_init() {
    register_isr_stubs();

    for(uint32_t i = 0; i < NUM_VECTORS - IRQ_OFFSET; i++) {
        list_init(&isrs[i]);
    }

    return 0;
}

pure_initcall(isr_init);
