#include <stdbool.h>
#include <stddef.h>

#include "lib/int.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "driver/console/console.h"
#include "video/log.h"
#include "misc/sysrq.h"

#define KEYBUFFLEN 512

#define KEYBOARD_IRQ 1

#define RELEASE_BIT (1 << 7)

static console_t *the_console;

static uint32_t keybuff_front = 0, keybuff_back = 0;
static uint8_t keybuff[KEYBUFFLEN];
static volatile uint32_t read_waiting = 0;
static DEFINE_SPINLOCK(keybuff_lock);
static DEFINE_SEMAPHORE(wait_semaphore, 0);

static bool key_state[128];
static const char key_map[] = {
          // 0x00
          0, 0,
          '\1', '\1',
          '1', '!',
          '2', '@',
          '3', '#',
          '4', '$',
          '5', '%',
          '6', '^',
          '7', '&',
          '8', '*',
          '9', '(',
          '0', ')',
          '-', '_',
          '=', '+',
          '\x7f', '\x7f', // backspace
          ' ', ' ',

          // 0x10
          'q', 'Q',
          'w', 'W',
          'e', 'E',
          'r', 'R',
          't', 'T',
          'y', 'Y',
          'u', 'U',
          'i', 'I',
          'o', 'O',
          'p', 'P',
          '[', '{',
          ']', '}',
          '\n', '\n',
            0, 0,
          'a', 'A',
          's', 'S',

          // 0x20
          'd', 'D',
          'f', 'F',
          'g', 'G',
          'h', 'H',
          'j', 'J',
          'k', 'K',
          'l', 'L',
          ';', ':',
          '\'', '\"',
          '`', '~',
          0, 0,
          '\\', '|',
          'z', 'Z',
          'x', 'X',
          'c', 'C',
          'v', 'V',

          // 0x30
          'b', 'B',
          'n', 'N',
          'm', 'M',
          ',', '<',
          '.', '>',
          '/', '?',
          0, 0,
          '*', '*',
          0, 0,
          ' ', ' ',
          0, 0,
          0, 0,
          0, 0,
          0, 0,
          0, 0,
          0, 0,

          // 0x40
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          '7', '7',
          '\3', '\3',
          '9', '9',
          '-', '-',
          '4', '4',
          '5', '5',
          '6', '6',
          '+', '+',
          '1', '1',

          // 0x50
          '\5', '\5',
          '3', '3',
          '0', '0',
          '.', '.',
          0, 0
};

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
        chunk_len = MIN(chunk_len, len - coppied);

        memcpy(buff, &keybuff[keybuff_back], chunk_len);

        coppied += chunk_len;
        buff += chunk_len;
        keybuff_back = (keybuff_back + chunk_len) % KEYBUFFLEN;

        spin_unlock_irqstore(&keybuff_lock, flags);
    }

    return coppied;
}

static void dispatch(uint8_t code) {
    keybuff_append(code);

    bool press = !(code & RELEASE_BIT);
    code &= ~RELEASE_BIT;
    key_state[code] = press;

    if(press && key_state[SYSRQ_KEY] && code != SYSRQ_KEY) {
        sysrq_handle(code);
    }
}

static void handle_keyboard(interrupt_t *interrupt, void *data) {
    while(inb(0x64) & 2);
    dispatch(inb(0x60));
}

void keyboard_init(console_t *console) {
    the_console = console;

    outb(0x64, 0xAA);
    if(inb(0x60) != 0x55) {
        kprintf("kbd - controller failed BIST!");
    }

    register_isr(KEYBOARD_IRQ + IRQ_OFFSET, CPL_KRNL, handle_keyboard, NULL);
}
