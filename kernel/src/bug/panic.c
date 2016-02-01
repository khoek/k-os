#include <stdbool.h>
#include "lib/string.h"
#include "lib/printf.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "sync/spinlock.h"
#include "sched/proc.h"
#include "log/log.h"
#include "driver/console/console.h"

#define MAX_FRAMES      32
#define PANICF_BUFF_SIZE        512

void panic(char *message) {
    cli();

    static volatile bool panic_in_progress = false;
    if(panic_in_progress) {
        die();
    }
    panic_in_progress = true;

    vram_color(con_global, 0x0C);
    vram_puts(con_global, "\nKERNEL PANIC (");
    if(get_percpu_ptr() && get_percpu_unsafe(this_proc)) {
        vram_putsf(con_global, "core %u", get_percpu_unsafe(this_proc)->num);
    } else {
        vram_puts(con_global, "invalid percpu ptr");
    }
    vram_puts(con_global, "): ");

    vram_color(con_global, 0x07);
    vram_putsf(con_global, "%s\n\n", message);

    con_global = NULL;

    vram_puts(con_global, "Stack trace:\n");
    uint32_t *ebp, eip = -1;
    asm("mov %%ebp, %0" : "=r" (ebp));
    eip = ebp[1] - 1;

    for(uint32_t frame = 0; eip && ebp && frame < MAX_FRAMES; frame++) {
        const elf_symbol_t *symbol = debug_lookup_symbol(eip);
        if(symbol) {
            vram_putsf(con_global, "    %s+0x%X/0x%X\n", debug_symbol_name(symbol), eip - symbol->value, eip);
        } else {
            vram_putsf(con_global, "    0x%X\n", eip);
        }

        ebp = (uint32_t *) ebp[0];
        eip = ebp[1] - 1;
    }

    die();
}

void panicf(char *fmt, ...) {
    char buff[PANICF_BUFF_SIZE];
    va_list va;
    va_start(va, fmt);
    if(vsprintf(buff, fmt, va) > PANICF_BUFF_SIZE) {
        panic("panicf overflow");
    }
    va_end(va);

    panic(buff);
}
