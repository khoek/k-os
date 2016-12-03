#include "init/initcall.h"
#include "bug/debug.h"
#include "arch/pit.h"
#include "arch/tsc.h"
#include "log/log.h"

static uint64_t initial;

static uint64_t tsc_read() {
    return rdtsc() - initial;
}

clock_t tsc_clock = {
    .name = "tsc",
    .rating = 7,

    .freq = 0,
    .read = tsc_read,
};

void tsc_busywait_100ms() {
    BUG_ON(!tsc_clock.freq);
    barrier();
    uint64_t then = rdtsc();
    barrier();
    uint64_t now = rdtsc();
    while(10 * (now - then) < tsc_clock.freq) {
        barrier();
        now = rdtsc();
        barrier();
    }
    barrier();
}

void __init tsc_init() {
    uint32_t n, d, crystal_freq;
    asm volatile("cpuid" : "=a"(d), "=b"(n), "=c"(crystal_freq)
        : "a"(0x80000007) : "edx");

    if(d && crystal_freq) {
        tsc_clock.freq = crystal_freq * n / d;

        kprintf("tsc - cpuid init %u/%u*%u (%uHz)", n, d, crystal_freq,
            tsc_clock.freq);
    } else {
        barrier();
        uint64_t then = rdtsc();
        barrier();
        for(uint32_t i = 0; i < 5; i++) {
            pit_busywait_20ms();
        }
        barrier();
        tsc_clock.freq = (rdtsc() - then) * 10;
        barrier();

        kprintf("tsc - pit init (%uHz)", tsc_clock.freq);
    }

    initial = rdtsc();

    register_clock(&tsc_clock);
}
