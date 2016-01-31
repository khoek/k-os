#include <stdbool.h>

#include "common/types.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "driver/console/console.h"
#include "log/log.h"

#define KEYBUFFLEN 512

#define KEYBOARD_IRQ 1

static console_t *the_console;

static uint32_t keybuff_front = 0, keybuff_back = 0;
static uint8_t keybuff[KEYBUFFLEN];
static volatile uint32_t read_waiting = 0;
static DEFINE_SPINLOCK(keybuff_lock);
static DEFINE_SEMAPHORE(wait_semaphore, 0);

static inline void keybuff_append(uint8_t code) {
    uint32_t flags;
    spin_lock_irqsave(&keybuff_lock, &flags);

    uint32_t new_front = (keybuff_front + 1) % KEYBUFFLEN;
    if(new_front == keybuff_back) goto append_out;

    keybuff[keybuff_front] = code;
    keybuff_front = new_front;

    if(read_waiting) {
        semaphore_up(&wait_semaphore);

        read_waiting--;
    }

append_out:
    spin_unlock_irqstore(&keybuff_lock, flags);
}

ssize_t keybuff_read(char *buff, size_t len) {
    uint32_t coppied = 0;

    while(coppied < len) {
        uint32_t flags;
        spin_lock_irqsave(&keybuff_lock, &flags);

        if(keybuff_front == keybuff_back) {
read_retry:
            read_waiting++;

            spin_unlock_irqstore(&keybuff_lock, flags);

            semaphore_down(&wait_semaphore);

            spin_lock_irqsave(&keybuff_lock, &flags);

            if(keybuff_front == keybuff_back) {
                    goto read_retry;
            }
        }

        uint32_t chunk_len = keybuff_front < keybuff_back ? KEYBUFFLEN - keybuff_back : keybuff_front - keybuff_back;
        uint32_t left = len - coppied;
        chunk_len = MIN(chunk_len, left);

        memcpy(buff, &keybuff[keybuff_back], chunk_len);

        coppied += chunk_len;
        buff += chunk_len;
        keybuff_back = (keybuff_back + chunk_len) % KEYBUFFLEN;

        spin_unlock_irqstore(&keybuff_lock, flags);
    }

    return coppied;
}

#define CTRL_REG 0x64
#define ENCR_REG 0x60

#define CTRL_STATUS_INBUF (1 << 1)

static void ctrl_send_cmd(uint8_t cmd) {
	while(inb(CTRL_REG) & CTRL_STATUS_INBUF);
	outb(CTRL_REG, cmd);
}

static uint8_t ctrl_read_status() {
	while(inb(CTRL_REG) & CTRL_STATUS_INBUF);
	return inb(ENCR_REG);
}

static void encr_send_cmd(uint8_t cmd) {
	while(inb(CTRL_REG) & CTRL_STATUS_INBUF);
	outb(ENCR_REG, cmd);
}

uint8_t encr_read() {
	while(inb(CTRL_REG) & CTRL_STATUS_INBUF);
	return inb(ENCR_REG);
}

static void handle_keyboard(interrupt_t *interrupt, void *data) {
    keybuff_append(encr_read());
}

void keyboard_init(console_t *console) {
    the_console = console;

    ctrl_send_cmd(0xAE);
    encr_read();
    ctrl_send_cmd(0x20);
    encr_read();

    encr_send_cmd(0xFF); //Reset and start self-test
    encr_read(); //0xFA = ACK
    encr_read(); //0xAA = SUCCESS

    encr_send_cmd(0xEE); //ECHO
    encr_read(); //0xEE = ECHO

    ctrl_send_cmd(0xAA); //Test PS/2 Controller
    if(ctrl_read_status() != 0x55) { //0x55 = SUCCESS
        kprintf("kbd - controller failed BIST!");
    }

    encr_send_cmd(0xF4);
    encr_read();
    encr_read();

    register_isr(KEYBOARD_IRQ + IRQ_OFFSET, CPL_KRNL, handle_keyboard, NULL);
}
