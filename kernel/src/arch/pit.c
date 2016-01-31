#include "common/types.h"
#include "common/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "time/clock.h"
#include "log/log.h"

#define TIMER_FREQ 1000

#define PIT_IRQ 0
#define PIT_CLOCK 1193182

static volatile uint64_t ticks;

static uint64_t pit_read() {
    return ticks;
}

static clock_t pit_clock = {
    .name = "pit",
    .rating = 5,

    .freq = TIMER_FREQ,
    .read = pit_read,
};

static clock_event_source_t pit_clock_event_source = {
    .name = "pit",
    .rating = 10,

    .freq = TIMER_FREQ,
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

static void handle_pit(interrupt_t *interrupt, void *data) {
    ticks++;

    pit_clock_event_source.event(&pit_clock_event_source);
}

void __init pit_init() {
    kprintf("pit - fallback timer registered");

    outb(0x43, 0x36);
    outb(0x40, (PIT_CLOCK / TIMER_FREQ) & 0xff);
    outb(0x40, ((PIT_CLOCK / TIMER_FREQ) >> 8) & 0xff);

    register_isr(PIT_IRQ + IRQ_OFFSET, CPL_KRNL, handle_pit, NULL);

    register_clock(&pit_clock);
    register_clock_event_source(&pit_clock_event_source);
}
