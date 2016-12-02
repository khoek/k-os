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

#define PANICF_BUFF_SIZE        512

volatile bool panic_in_progress = false;

void panic(char *message) {
    irqdisable();
    breakpoint();

    if(panic_in_progress || !con_global) {
        die();
    }
    ACCESS_ONCE(panic_in_progress) = true;
    barrier();

    dispatch_management_interrupts();

    console_t *t = con_global;

    console_lockup(t);

    vram_color(t, 0x0C);
    vram_puts(t, "\nKERNEL PANIC (");
    if(percpu_up && get_percpu_ptr() && get_percpu(this_proc)) {
        vram_putsf(t, "core %u", get_percpu(this_proc)->num);
    } else {
        vram_puts(t, "invalid percpu ptr");
    }
    vram_puts(t, "): ");

    vram_color(t, 0x07);
    vram_putsf(t, "%s\n", message);

    dump_stack_trace(t);

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
