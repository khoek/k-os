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
#include "arch/pit.h"
#include "mm/cache.h"
#include "sched/sched.h"
#include "video/log.h"

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

static void pic_eoi(uint32_t vector) {
    if(vector > PIC_SLAVE_OFFSET + PIC_NUM_PINS) return;

    if(vector >= PIC_SLAVE_OFFSET && vector != INT_SPURIOUS_SLAVE) {
        outb(SLAVE_COMMAND , EOI);
    }

    if(vector != INT_SPURIOUS_MASTER) {
        outb(MASTER_COMMAND, EOI);
    }
}

void __init pic_init() {
    eoi_handler = pic_eoi;

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
}
