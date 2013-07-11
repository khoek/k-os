#include <stdarg.h>

#include "lib/string.h"
#include "lib/printf.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "mm/mm.h"
#include "video/console.h"

#define BUFFSIZE 1024

#define VRAM_PHYS 0xb8000
#define PORT_BASE 0x463

static uint32_t row, col;
static uint16_t *vga_port = ((uint16_t *) PORT_BASE);
static char *vram = ((char *) VRAM_PHYS);
static char color = 0x07;

static SPINLOCK_INIT(vram_lock);
static SPINLOCK_INIT(buffer_lock);

void console_map_virtual() {
    vga_port = mm_map(vga_port);
    vram = mm_map(vram);
}

uint8_t console_row() {
     return row;
}

uint8_t console_col() {
     return col;
}

void console_color(const char c) {
     color = c;
}

void console_clear() {
    uint32_t flags;
    spin_lock_irqsave(&vram_lock, &flags);

    for(uint32_t i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        vram[i * 2] = ' ';
        vram[i * 2 + 1] = 0x7;
    }
    col = 0;
    row = 0;

    spin_unlock_irqstore(&vram_lock, flags);
}

void console_write(const char *str, const uint8_t len) {
    uint32_t flags;
    spin_lock_irqsave(&vram_lock, &flags);

    for (int i = 0; i < len; i++) {
        if(str[i] == '\n') {
            col = 0;
            row++;
        } else if(str[i] == '\r') {
        col =    0;
        } else {
            vram[(row * CONSOLE_WIDTH + col) * 2] = str[i];
            vram[(row * CONSOLE_WIDTH + col) * 2 + 1] = color;
            col++;
        }

        if(col == CONSOLE_WIDTH) {
            col = 0;
            row++;
        }

        if(row == CONSOLE_HEIGHT) {
            memmove(vram, vram + CONSOLE_WIDTH * 2, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2);

            for(int i = 0; i < 80; i++) {
                (vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2] = ' ';
                (vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2 + 1] = 7;
            }

            row = CONSOLE_HEIGHT - 1;
        }
    }

    spin_unlock_irqstore(&vram_lock, flags);

    console_cursor(row, col);
}

void console_cursor(const uint8_t r, const uint8_t c) {
    if (r >= CONSOLE_HEIGHT || c >= CONSOLE_WIDTH) {
        row = 0;
        col = 0;
    } else {
        row = r;
        col = c;
    }

    uint16_t base_vga_port = *vga_port; // read base vga port from bios data

    uint32_t flags;
    spin_lock_irqsave(&vram_lock, &flags);

    outb(base_vga_port, 0x0e);
    outb(base_vga_port + 1, ((row * CONSOLE_WIDTH + col) >> 8) & 0xff);

    outb(base_vga_port, 0x0f);
    outb(base_vga_port + 1, (row * CONSOLE_WIDTH + col) & 0xff);

    spin_unlock_irqstore(&vram_lock, flags);
}

void console_puts(const char* str) {
    console_write(str, strlen(str));
}

static char buff[BUFFSIZE];
void console_putsf(const char* fmt, ...) {
    uint32_t flags;
    spin_lock_irqsave(&buffer_lock, &flags);

    va_list va;
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);

    console_puts(buff);

    spin_unlock_irqstore(&buffer_lock, flags);
}
