#include "lib/int.h"
#include "common/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "time/clock.h"
#include "video/log.h"

#define TIMER_FREQ 1000

#define PIT_CLOCK 1193182

/*
    The following code shuts down the PIT,
    it will probably be useful at some point.

    idt_unregister(32);
    outb_pit(0x43, 0x30);
    outb_pit(0x40, 0);
    outb_pit(0x40, 0);
*/

static uint64_t ticks;
static uint64_t pit_read() {
    return ticks;
}

static void handle_pit(interrupt_t UNUSED(*interrupt));

static clock_t pit_clock = {
    .name = "pit",
    .rating = 5,

    .read = pit_read
};

static clock_event_source_t pit_clock_event_source = {
    .name = "pit",
    .rating = 10,

    .freq = TIMER_FREQ
};

void play(uint32_t freq) {
    uint32_t divisor = PIT_CLOCK / freq;

    outb(0x43, 0xB6);
    outb(0x42, divisor & 0xff);
    outb(0x42, (divisor >> 8) & 0xff);

    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

void stop() {
    outb(0x61, inb(0x61) & 0xFC);
}

void beep() {
    play(1000);
    sleep(10);
    stop();
}

static void handle_pit(interrupt_t UNUSED(*interrupt)) {
    ticks++;

    pit_clock_event_source.event(&pit_clock_event_source);
}

static INITCALL pit_init() {
    register_clock(&pit_clock);
    register_clock_event_source(&pit_clock_event_source);

    idt_register(32, CPL_KERNEL, handle_pit);
    outb(0x43, 0x36);
    outb(0x40, (PIT_CLOCK / TIMER_FREQ) & 0xff);
    outb(0x40, ((PIT_CLOCK / TIMER_FREQ) >> 8) & 0xff);

    logf("pit - setting freq to %uHZ", TIMER_FREQ);

    return 0;
}

arch_initcall(pit_init);
