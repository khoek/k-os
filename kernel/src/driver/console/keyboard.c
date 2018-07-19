#include <stdbool.h>

#include "common/types.h"
#include "common/asm.h"
#include "common/ringbuff.h"
#include "init/initcall.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "bug/panic.h"
#include "sched/sched.h"
#include "driver/console/console.h"
#include "driver/console/tty.h"
#include "log/log.h"

#define KEYBUFFLEN 100

#define KEYBOARD_IRQ 1

#define KBD_SPECIAL_CHAR 0xE0

static console_t *the_console;

static uint8_t keybuf[KEYBUFFLEN];
static DEFINE_RINGBUFF(keyrb, keybuf);
static DEFINE_SPINLOCK(keybuff_lock);

static inline void keybuff_append(uint8_t code) {
    uint32_t flags;
    spin_lock_irqsave(&keybuff_lock, &flags);

    if(ringbuff_is_full(&keyrb, uint8_t)) {
        panic("keybuff full");
    }

    ringbuff_write(&keyrb, &code, 1, uint8_t);

    spin_unlock_irqstore(&keybuff_lock, flags);

    if(code != KBD_SPECIAL_CHAR) {
        tty_notify();
    }
}

//read at most len bytes from the key buffer, NEVER blocking
ssize_t keybuff_read(uint8_t *buff, size_t len) {
    int32_t ret = 0;

    uint32_t flags;
    spin_lock_irqsave(&keybuff_lock, &flags);
    ret = ringbuff_read(&keyrb, buff, len, uint8_t);
    spin_unlock_irqstore(&keybuff_lock, flags);

    return ret;
}

bool keybuff_is_empty() {
    bool ret = 0;

    uint32_t flags;
    spin_lock_irqsave(&keybuff_lock, &flags);
    ret = ringbuff_is_empty(&keyrb, uint8_t);
    spin_unlock_irqstore(&keybuff_lock, flags);

    return ret;
}

#define CMD_REG 0x64
#define DATA_REG 0x60

//the buffers are only one byte, so full means data present
#define STATUS_OUTBUF_FULL (1 << 0)
#define STATUS_INBUF_FULL  (1 << 1)

void keyboard_poll() {
    if(!(inb(CMD_REG) & STATUS_OUTBUF_FULL)) {
        return;
    }

    keybuff_append(inb(DATA_REG));
}

static void handle_keyboard(interrupt_t *interrupt, void *data) {
    keyboard_poll();
}

void keyboard_init(console_t *console) {
    the_console = console;

    register_isr(KEYBOARD_IRQ + IRQ_OFFSET, CPL_KRNL, handle_keyboard, NULL);
}
