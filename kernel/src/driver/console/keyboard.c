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
#include "driver/console/console.h"
#include "log/log.h"

#define KEYBUFFLEN 2048

#define KEYBOARD_IRQ 1

static console_t *the_console;

static uint8_t keybuf[KEYBUFFLEN];
static DEFINE_RINGBUFF(keyrb, keybuf);
static DEFINE_SPINLOCK(keybuff_lock);

static volatile uint32_t read_waiting = 0;
static DEFINE_SEMAPHORE(wait_semaphore, 0);

static inline void keybuff_append(uint8_t code) {
    uint32_t flags;
    spin_lock_irqsave(&keybuff_lock, &flags);

    if(ringbuff_is_full(&keyrb, uint8_t)) {
        panic("keybuff full");
    }

    ringbuff_write(&keyrb, &code, 1, uint8_t);
    if(read_waiting) {
        semaphore_up(&wait_semaphore);
    }

    spin_unlock_irqstore(&keybuff_lock, flags);
}

//read at most len bytes from the key buffer, blocking only if it is empty
ssize_t keybuff_read(uint8_t *buff, size_t len) {
    uint32_t flags;
    spin_lock_irqsave(&keybuff_lock, &flags);

    read_waiting++;
    while(ringbuff_is_empty(&keyrb, uint8_t)) {
        spin_unlock_irqstore(&keybuff_lock, flags);
        semaphore_down(&wait_semaphore);
        spin_lock_irqsave(&keybuff_lock, &flags);
    }
    read_waiting--;
    ssize_t ret = ringbuff_read(&keyrb, buff, len, uint8_t);

    spin_unlock_irqstore(&keybuff_lock, flags);

    return ret;
}

#define CTRL_REG 0x64
#define ENCR_REG 0x60

#define CTRL_STATUS_INBUF (1 << 1)

uint8_t encr_read() {
    //FIXME potential infinite loop?
    while(inb(CTRL_REG) & CTRL_STATUS_INBUF) {
        //wait
    }
    return inb(ENCR_REG);
}

static void handle_keyboard(interrupt_t *interrupt, void *data) {
    keybuff_append(encr_read());
}

void keyboard_init(console_t *console) {
    the_console = console;

    register_isr(KEYBOARD_IRQ + IRQ_OFFSET, CPL_KRNL, handle_keyboard, NULL);
}
