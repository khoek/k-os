#include "common/types.h"
#include "common/asm.h"
#include "lib/printf.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "time/timer.h"
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

#define SPKR_ENABLE  0x3
#define SPKR_DISABLE 0x0

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
static uint32_t spkr_freq = 0;

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

#define BUSYWAIT_INITAL 60000
#define BUSYWAIT_THRESHOLD 23864

void pit_busywait_20ms() {
    set_counter(SEL_C0, MD_2, BUSYWAIT_INITAL);
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

#define SPKR_RBUFF_LEN 100
#define SPKR_WBUFF_LEN 100

static DEFINE_SPINLOCK(pit_lock);
static uint32_t wbuff_front = 0, rbuff_front = 0, rbuff_len = 0;
static char spkr_rbuff[SPKR_RBUFF_LEN];
static char spkr_wbuff[SPKR_WBUFF_LEN];

//called under pit_lock
static void reload_spkr_freq() {
    //FIXME use vsnprintf to prevent buffer overflow
    wbuff_front = 0;
    rbuff_front = 0;
    rbuff_len = sprintf(spkr_rbuff, "%u\n", spkr_freq);
    BUG_ON(rbuff_len + 1 >= SPKR_RBUFF_LEN);

    if(spkr_freq) {
        set_counter(SEL_C2, MD_3, PIT_CLOCK / spkr_freq);

        outb(REG_KBD, SPKR_ENABLE);
    } else {
        outb(REG_KBD, SPKR_DISABLE);
    }
}

void spkr_set_freq(uint32_t freq) {
    uint32_t flags;
    spin_lock_irqsave(&pit_lock, &flags);

    spkr_freq = freq;
    reload_spkr_freq();

    spin_unlock_irqstore(&pit_lock, flags);
}

static void end_beep(void *unused) {
    spkr_set_freq(0);
}

void beep() {
    spkr_set_freq(1000);
    
    //FIXME make sure we aren't being called too early to use a timer
    timer_create(10, (void (*)(void *)) end_beep, NULL);
}

static ssize_t spkr_char_read(char_device_t UNUSED(*cdev), char *buff, size_t len) {
    uint32_t flags;
    spin_lock_irqsave(&pit_lock, &flags);

    size_t left = len;
    while(left--) {
        *buff++ = spkr_rbuff[rbuff_front++];
        rbuff_front %= rbuff_len;
    }

    spin_unlock_irqstore(&pit_lock, flags);

    return len;
}

static ssize_t spkr_char_write(char_device_t *cdev, const char *buff, size_t len) {
    uint32_t flags;
    spin_lock_irqsave(&pit_lock, &flags);

    size_t left = len;
    while(left--) {
        spkr_wbuff[wbuff_front++] = *buff;
        if(*buff == '\n') {
            spkr_wbuff[wbuff_front] = '\0';
            spkr_freq = atoi(spkr_wbuff);
            reload_spkr_freq();
        } else if(wbuff_front == SPKR_WBUFF_LEN - 1) {
            wbuff_front = 0;
        }

        buff++;
    }

    spin_unlock_irqstore(&pit_lock, flags);

    return len;
}

static char_device_ops_t spkr_ops = {
    .read = spkr_char_read,
    .write = spkr_char_write,
};

void __init pit_init() {
    //Initialize "PIT" (counter 0)
    set_counter(SEL_C0, MD_3, PIT_CLOCK / TIMER_FREQ);
    register_isr(PIT_IRQ + IRQ_OFFSET, CPL_KRNL, handle_pit, NULL);
    register_clock(&pit_clock);
    register_clock_event_source(&pit_clock_event_source);

    //Initialize SPKR (counter 2)
    spkr_freq = 0;
    reload_spkr_freq();
    char_device_t *spkr = char_device_alloc();
    spkr->ops = &spkr_ops;
    register_char_device(spkr, "spkr");

    kprintf("pit - timer registered");
}
