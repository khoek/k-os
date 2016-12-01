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

#define MAX_FRAMES              32
#define PANICF_BUFF_SIZE        512

static inline uint32_t get_address(thread_t *me, uint32_t vaddr, uint32_t off) {
    if(!virt_is_valid(me, (void *) vaddr)) {
        return 0;
    }
    return ((uint32_t *) vaddr)[off];
}

void panic(char *message) {
    irqdisable();
    breakpoint();

    static volatile bool panic_in_progress = false;
    if(panic_in_progress || !con_global) {
        die();
    }
    ACCESS_ONCE(panic_in_progress) = true;
    barrier();

    console_t *t = con_global;

    vram_color(t, 0x0C);
    vram_puts(t, "\nKERNEL PANIC (");
    if(get_percpu_ptr() && get_percpu(this_proc)) {
        vram_putsf(t, "core %u", get_percpu(this_proc)->num);
    } else {
        vram_puts(t, "invalid percpu ptr");
    }
    vram_puts(t, "): ");

    vram_color(t, 0x07);
    vram_putsf(t, "%s\n", message);

    thread_t *me = current;
    if(me) {
        vram_putsf(t, "Current task: %s (%u)\n", me->node->argv[0], me->node->pid);
    } else {
        vram_puts(t, "Current task: NULL\n");
    }

    vram_puts(t, "Stack trace:\n");
    uint32_t last_ebp = 0, ebp, eip;
    asm("mov %%ebp, %0" : "=r" (ebp));
    eip = get_address(me, ebp, 1);

    uint32_t top = (uint32_t) me->kernel_stack_top;
    uint32_t bot = (uint32_t) me->kernel_stack_bottom;
    for(uint32_t frame = 0; eip && ebp && ebp > last_ebp && (!me || (ebp >= top && ebp <= bot)) && frame < MAX_FRAMES; frame++) {
        const elf_symbol_t *symbol = debug_lookup_symbol(eip - 1);
        if(symbol) {
            vram_putsf(t, "    %s+0x%X/0x%X\n", debug_symbol_name(symbol), eip - 1 - symbol->value, eip - 1);
        } else {
            vram_putsf(t, "    0x%X\n", eip - 1);
        }

        last_ebp = ebp;
        ebp = get_address(me, ebp, 0);
        eip = get_address(me, ebp, 1);
    }

    vram_puts(t, "Stack trace end");

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
