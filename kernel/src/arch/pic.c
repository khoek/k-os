#include "init/initcall.h"
#include "common/compiler.h"
#include "common/asm.h"
#include "common/list.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "arch/gdt.h"
#include "arch/pl.h"
#include "arch/pit.h"
#include "mm/cache.h"
#include "sched/sched.h"
#include "log/log.h"

//command io port of PICs
#define MASTER_COMMAND 0x20
#define SLAVE_COMMAND  0xA0

//data io port of PICs
#define MASTER_DATA 0x21
#define SLAVE_DATA  0xA1

//PIC commands
#define INIT 0x11 //enter "init" mode
#define EOI  0x20 //end of interrupt handler

#define PIC_REG_IRR 0x0A
#define PIC_REG_ISR 0x0B

static bool pic_is_spurious(uint32_t vector) {
    if(vector - IRQ_OFFSET != IRQ_SPURIOUS) return false;

    outb(MASTER_COMMAND, PIC_REG_ISR);

    return vector - IRQ_OFFSET == IRQ_SPURIOUS
        && !(inb(MASTER_COMMAND) & (1 << IRQ_SPURIOUS));
}

static void pic_eoi(uint32_t vector) {
    if(vector > PIC_SLAVE_OFFSET + PIC_NUM_PINS) return;
    if(pic_is_spurious(vector)) return;

    //don't ignore INT_SPURIOUS_MASTER interrupts because that is the IDE
    //secondary IRQ vector
    if(vector >= PIC_SLAVE_OFFSET) {
        outb(SLAVE_COMMAND, EOI);
    }

    if(vector != INT_SPURIOUS_MASTER) {
        outb(MASTER_COMMAND, EOI);
    }
}

void __init pic_configure(uint8_t master_mask, uint8_t slave_mask) {
    //send INIT command
    outb(MASTER_COMMAND, INIT);
    outb(SLAVE_COMMAND , INIT);

    //set interrupt vector offset
    outb(MASTER_DATA, PIC_MASTER_OFFSET);
    outb(SLAVE_DATA , PIC_SLAVE_OFFSET);

    //set master/slave status
    outb(MASTER_DATA, 4);
    outb(SLAVE_DATA , 2);

    //set mode
    outb(MASTER_DATA, 0x01);
    outb(SLAVE_DATA , 0x01);

    //set IRQ masks
    outb(MASTER_DATA, master_mask);
    outb(SLAVE_DATA , slave_mask);
}

void __init pic_init() {
    eoi_handler = pic_eoi;
    is_spurious = pic_is_spurious;

    pic_configure(0, 0);

    sti();
}
