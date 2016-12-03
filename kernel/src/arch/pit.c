#include "common/types.h"
#include "common/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "time/clock.h"
#include "log/log.h"

#define TIMER_FREQ 1000

#define PIT_IRQ 0
#define PIT_CLOCK 1193182

//Registers
#define REG_KBD     0x61
#define REG_CMD     0x43
#define REG_CNTBASE 0x40
#define REG_CNT1 (REG_CNTBASE + 0x0)
#define REG_CNT2 (REG_CNTBASE + 0x1)
#define REG_CNT3 (REG_CNTBASE + 0x2)

#define SPKR_ENABLE 0x3

//Shifts for command words
#define SHIFT_SC  6
#define SHIFT_AC  4
#define SHIFT_MD  1
#define SHIFT_BCD 0

//Select Counter (bits 6-7)
#define SEL_C0 0x0
#define SEL_C1 0x1
#define SEL_C2 0x2
//Special Read-Back command
#define CMD_RDB 0x3

//CMD_RDB options (bits 4-5)
#define NO_LATCH_COUNT   0x2
#define NO_LATCH_STATUS  0x1

//CMD_RDB counter select (bits 1-3)
#define RDB_BASE_SHIFT 1

//Access mode for SEL != CMD_RDB (bits 4-5)
#define CMD_LATCH   0x0
#define RW_LO   0x1
#define RW_HI   0x2
#define RW_LOHI 0x3

//Operating mode (bits 1-3)
//Interrupt on Terminal Count
#define MD_0 0x0
//Hardware Retriggerable One-Shot
#define MD_1 0x1
//Rate Generator
#define MD_2 0x2
//Square Wave Mode
#define MD_3 0x3
//Software Triggered Mode
#define MD_4 0x4
//Hardware Triggered Strobe (Retriggerable)
#define MD_5 0x5
//Duplicate commands
#define MD_2DUP 0x6
#define MD_3DUP 0x7

//BCD Enable
#define BCD_OFF 0x0
#define CMD_BCD_ON  0x1

static volatile uint64_t ticks;

static uint64_t pit_read() {
    return ticks;
}

static inline uint8_t make_cmd(uint8_t sc, uint8_t ac, uint8_t md,
    uint8_t bcd) {
    return (sc << SHIFT_SC)
        | (ac << SHIFT_AC)
        | (md << SHIFT_MD)
        | (bcd << SHIFT_BCD);
}

static inline uint8_t make_rdb(uint8_t ops, uint8_t counts) {
    return (CMD_RDB << SHIFT_SC)
        | (ops << SHIFT_AC)
        | (1 << (counts + RDB_BASE_SHIFT));
}

static inline void set_counter(uint8_t sc, uint8_t md, uint16_t val) {
    outb(REG_CMD, make_cmd(sc, RW_LOHI, md, BCD_OFF));
    outb(REG_CNTBASE + sc, val & 0xff);
    outb(REG_CNTBASE + sc, val >> 8);
}

static inline uint16_t read_counter(uint8_t sc) {
    outb(REG_CMD, make_rdb(NO_LATCH_STATUS, sc));
    uint16_t res = inb(REG_CNTBASE + sc);
    res |= ((uint16_t) inb(REG_CNTBASE + sc)) << 8;
    return res;
}

static clock_t pit_clock = {
    .name = "pit",
    .rating = 5,

    .freq = TIMER_FREQ,
    .read = pit_read,
};

static clock_event_source_t pit_clock_event_source = {
    .name = "pit",
    .rating = 7,

    .freq = TIMER_FREQ,
};

void play(uint32_t freq) {
    set_counter(SEL_C2, MD_3, PIT_CLOCK / freq);

    uint8_t val = inb(REG_KBD);
    if((val & SPKR_ENABLE) != SPKR_ENABLE) {
        outb(REG_KBD, val | SPKR_ENABLE);
    }
}

void stop() {
    outb(REG_KBD, inb(REG_KBD) & SPKR_ENABLE);
}

void beep() {
    play(1000);
    sleep(10);
    stop();
}

#define BUSYWAIT_INITAL 60000
#define BUSYWAIT_THRESHOLD 23864

void pit_busywait_20ms() {
    barrier();
    set_counter(SEL_C0, MD_3, BUSYWAIT_INITAL);
    barrier();
    uint16_t ret = read_counter(SEL_C0);
    barrier();
    while(BUSYWAIT_INITAL - ret < BUSYWAIT_THRESHOLD) {
        barrier();
        ret = read_counter(SEL_C0);
        barrier();
    }
}

static void handle_pit(interrupt_t *interrupt, void *data) {
    ACCESS_ONCE(ticks)++;

    pit_clock_event_source.event(&pit_clock_event_source);
}

void __init pit_init() {
    set_counter(SEL_C0, MD_3, PIT_CLOCK / TIMER_FREQ);

    register_isr(PIT_IRQ + IRQ_OFFSET, CPL_KRNL, handle_pit, NULL);

    register_clock(&pit_clock);
    register_clock_event_source(&pit_clock_event_source);

    kprintf("pit - timer registered");
}
