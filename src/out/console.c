#include <stdarg.h>

#include "string.h"
#include "printf.h"
#include "console.h"
#include "panic.h"
#include "mm.h"
#include "asm.h"

#define BUFFSIZE 1024

#define VRAM_PHYS 0xb8000
#define PORT_BASE 0x463

static uint32_t row, col;
static uint16_t *vga_port = ((uint16_t *) PORT_BASE);
static char *vram = ((char *) VRAM_PHYS);
static char color = 0x07;

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
     unsigned int i;
     for(i = 0; i < (80 * 25) * 2; i += 2) {
          vram[i] = ' ';
          vram[i + 1] = 7;
     }
     col = 0;
     row = 0;
}

void console_write(const char *str, const uint8_t len) {
     for (int i = 0; i < len; i++) {
          if(str[i] == '\n') {
                col = 0;
                row++;
          } else if(str[i] == '\r') {
                col = 0;
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

          console_cursor(row, col);
     }
}

void console_cursor(const uint8_t r, const uint8_t c) {
     if (r >= CONSOLE_HEIGHT || c >= CONSOLE_WIDTH) {
          panicf("illegal cursor location %u, %u", r, c);
     }

     row = r;
     col = c;

     uint16_t base_vga_port = *vga_port; // read base vga port from bios data

     outb(base_vga_port, 0x0e);
     outb(base_vga_port + 1, ((row * CONSOLE_WIDTH + col) >> 8) & 0xff);

     outb(base_vga_port, 0x0f);
     outb(base_vga_port + 1, (row * CONSOLE_WIDTH + col) & 0xff);
}

void console_puts(const char* str) {
     console_write(str, strlen(str));
}

static char buff[BUFFSIZE];
void console_putsf(const char* fmt, ...) {
     va_list va;
     va_start(va, fmt);
     vsprintf(buff, fmt, va);
     va_end(va);

     console_puts(buff);
}

void console_relocate_vram() {
    ;
}
