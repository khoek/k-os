#include <stdbool.h>
#include <stdarg.h>
#include <printf.h>
#include "panic.h"
#include "idt.h"
#include "console.h"

#define MAX_FRAMES  32
#define BUFFSIZE    512

void die() {
    cli();
    while(true) hlt();
}

void panic(char* message) {
    cli();

    console_color(0xC7);
    kprintf("\n\nKernel Panic - %s\n", message);
    console_color(0x07);
 
    unsigned int * ebp;
    asm("mov %%ebp,%0" : "=r"(ebp));

    for(uint16_t frame = 0; frame < MAX_FRAMES; frame++) {
        uint32_t eip = ebp[1];
        if(eip == 0)
            break;
        ebp = (unsigned int *)(ebp[0]);
        //unsigned int * arguments = &ebp[2];
        kprintf("    0x%X     \n", eip);
    }

    while(true) hlt();
}

void panicf(char* fmt, ...) {
    char buff[BUFFSIZE];
    va_list va;
    va_start(va, fmt);
    if(vsprintf(buff, fmt, va) > BUFFSIZE) {
        panic("panicf overflow");
    }
    va_end(va);

    panic(buff);
}
