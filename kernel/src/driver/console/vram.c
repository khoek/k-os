#include <stdarg.h>

#include "lib/string.h"
#include "lib/printf.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "arch/bios.h"
#include "mm/mm.h"
#include "driver/console/console.h"

#define BUFFSIZE 1024

void vram_color(console_t *con, char c) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    con->color = c;

    spin_unlock_irqstore(&con->lock, flags);
}

void vram_clear(console_t *con) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    for(uint32_t i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        con->vram[i * 2] = ' ';
        con->vram[i * 2 + 1] = 0x7;
    }
    con->col = 0;
    con->row = 0;

    spin_unlock_irqstore(&con->lock, flags);
}

void vram_write(console_t *con, const char *str, size_t len) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    for (size_t i = 0; i < len; i++) {
        if(str[i] == '\n') {
            con->col = 0;
            con->row++;
        } else if(str[i] == '\r') {
            con->col = 0;
        } else {
            con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2] = str[i];
            con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2 + 1] = con->color;
            con->col++;
        }

        if(con->col == CONSOLE_WIDTH) {
            con->col = 0;
            con->row++;
        }

        if(con->row == CONSOLE_HEIGHT) {
            memmove(con->vram, con->vram + CONSOLE_WIDTH * 2, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2);

            for(int i = 0; i < 80; i++) {
                (con->vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2] = ' ';
                (con->vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2 + 1] = 7;
            }

            con->row = CONSOLE_HEIGHT - 1;
        }
    }

    spin_unlock_irqstore(&con->lock, flags);

    vram_cursor(con, con->row, con->col);
}

void vram_cursor(console_t *con, uint8_t r, uint8_t c) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    if (r >= CONSOLE_HEIGHT || c >= CONSOLE_WIDTH) {
        con->row = 0;
        con->col = 0;
    } else {
        con->row = r;
        con->col = c;
    }

    outb(con->port, 0x0e);
    outb(con->port + 1, ((con->row * CONSOLE_WIDTH + con->col) >> 8) & 0xff);

    outb(con->port, 0x0f);
    outb(con->port + 1, (con->row * CONSOLE_WIDTH + con->col) & 0xff);

    spin_unlock_irqstore(&con->lock, flags);
}

void vram_puts(console_t *con, const char* str) {
    vram_write(con, str, strlen(str));
}

//FIXME remove the need for this
static char buff[BUFFSIZE];
DEFINE_SPINLOCK(buffer_lock);
void vram_putsf(console_t *con, const char* fmt, ...) {
    uint32_t flags;
    spin_lock_irqsave(&buffer_lock, &flags);

    va_list va;
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);

    vram_puts(con, buff);

    spin_unlock_irqstore(&buffer_lock, flags);
}

void vram_init(console_t *console) {
    console->row = 0;
    console->col = 0;
    console->color = 0x07;
    console->vram = map_page(BIOS_VRAM);
    console->port = bda_getw(BDA_VRAM_PORT);
}
