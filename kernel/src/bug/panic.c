#include <stddef.h>
#include <stdbool.h>
#include "lib/string.h"
#include "lib/printf.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "sync/spinlock.h"
#include "sched/proc.h"
#include "video/log.h"
#include "video/console.h"

#define MAX_FRAMES      32
#define BUFFSIZE        512

void panic(char *message) {
    cli();

    static volatile bool panic_in_progress = false;
    if(panic_in_progress) {
        die();
    }
    panic_in_progress = true;

    spin_lock(&log_lock);

    console_color(0x0C);
    console_puts("\nKERNEL PANIC (");
    if(get_percpu_ptr() && get_percpu_unsafe(this_proc)) {
        console_putsf("core %u", get_percpu_unsafe(this_proc)->num);
    } else {
        console_puts("invalid percpu ptr");
    }
    console_puts("): ");

    console_color(0x07);
    console_putsf("%s\n\n", message);

    console_puts("Stack trace:\n");
    uint32_t *ebp, eip = -1;
    asm("mov %%ebp, %0" : "=r" (ebp));
    eip = ebp[1] - 1;

    for(uint32_t frame = 0; eip && ebp && frame < MAX_FRAMES; frame++) {
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
