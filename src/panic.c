#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <printf.h>
#include "panic.h"
#include "idt.h"
#include "elf.h"
#include "console.h"

#define MAX_FRAMES      32
#define BUFFSIZE        512

void die() {
    while(true) hlt();
}

void panic(char* message) {
    cli();

    console_clear();

    console_color(0x0C);
    kprintf("KERNEL PANIC: ");
    console_color(0x07);
    kprintf("%s\n\n", message);

    kprintf("Stack trace:\n");
    uint32_t *ebp, eip = -1;
    asm("mov %%ebp, %0" : "=r" (ebp));
    eip = ebp[1];

    for(uint32_t frame = 0; eip != 0 && ebp != 0 && frame < MAX_FRAMES; frame++) {
        elf_symbol_t *symbol = elf_lookup_symbol(eip);
        if(symbol == NULL) {
            kprintf("    0x%X\n", eip);
        } else {
            kprintf("    %s+0x%X\n", elf_symbol_name(symbol), eip - symbol->value);
        }

        ebp = (uint32_t *) ebp[0];
        eip = ebp[1];
    }

    die();
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
