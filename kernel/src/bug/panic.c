#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#include "lib/string.h"
#include "lib/printf.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "video/console.h"

#define MAX_FRAMES      32
#define BUFFSIZE        512

void panic(char *message) {
    cli();

    static bool once = false;
    if(once) {
        die();
    }
    once = true;

    console_color(0x0C);
    console_puts("\nKERNEL PANIC: ");
    console_color(0x07);
    console_putsf("%s\n\n", message);

    console_puts("Stack trace:\n");
    uint32_t *ebp, eip = -1;
    asm("mov %%ebp, %0" : "=r" (ebp));
    eip = ebp[1] - 1;

    for(uint32_t frame = 0; eip != 0 && ebp != 0 && frame < MAX_FRAMES; frame++) {
        const elf_symbol_t *symbol = debug_lookup_symbol(eip);
        if(symbol == NULL) {
            console_putsf("    0x%X\n", eip);
        } else {
            console_putsf("    %s+0x%X/0x%X\n", debug_symbol_name(symbol), eip - symbol->value, eip);
        }

        ebp = (uint32_t *) ebp[0];
        eip = ebp[1] - 1;
    }

    die();
}

void panicf(char *fmt, ...) {
    char buff[BUFFSIZE];
    va_list va;
    va_start(va, fmt);
    if(vsprintf(buff, fmt, va) > BUFFSIZE) {
        panic("panicf overflow");
    }
    va_end(va);

    panic(buff);
}
