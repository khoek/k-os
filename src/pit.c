#include <stdint.h>
#include "pit.h"
#include "common.h"
#include "idt.h"
#include "panic.h"
#include "io.h"
#include "console.h"

#define PIT_CLOCK 1193180

uint64_t ticks;
uint64_t uptime() {
    return ticks;
}

void play(uint32_t freq) {
    uint32_t divisor = 1193180 / freq;

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

void sleep(uint32_t milis) {
    uint64_t then = ticks;
    while(ticks - then < milis) hlt();
}

static void handle_pit(interrupt_t UNUSED(*interrupt)) {
    ticks++;
}

void pit_init(uint32_t freq) {
    idt_register(32, handle_pit);

    uint32_t divisor = PIT_CLOCK / freq;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xff);
    outb(0x40, (divisor >> 8) & 0xff);
}
