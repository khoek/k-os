#include <stdarg.h>
#include <string.h>
#include <printf.h>
#include "console.h"
#include "panic.h"
#include "io.h"

#define BUFFSIZE    1024
#define VRAM        ((char *) 0xb8000)

static uint32_t row, col;
static char color = 0x07;

uint8_t console_row() {
     return row;
}

uint8_t console_col() {
     return col;
}

void console_color(char c) {
     color = c;
}

void console_clear() {
     unsigned int i;
     for(i = 0; i < (80 * 25) * 2; i += 2) {
          VRAM[i] = ' ';
          VRAM[i + 1] = 7;
     }
     col = 0;
     row = 0;
}

void console_write(char *s, uint8_t len) {
     for (int i = 0; i < len; i++) {
          if(s[i] == '\n') {
                col = 0;
                row++;
          } else if(s[i] == '\r') {
                col = 0;
          } else {
                VRAM[(row * CONSOLE_WIDTH + col) * 2] = s[i];
                VRAM[(row * CONSOLE_WIDTH + col) * 2 + 1] = color;
                col++;
          }

          if(col == CONSOLE_WIDTH) {
                col = 0;
                row++;
          }

          if(row == CONSOLE_HEIGHT) {
                memmove(VRAM, VRAM + CONSOLE_WIDTH * 2, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2);

                for(int i = 0; i < 80; i++) {
                     (VRAM + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2] = ' ';
                     (VRAM + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2 + 1] = 7;
                }

                row = CONSOLE_HEIGHT - 1;
          }

          console_cursor(row, col);
     }
}

void console_puts(char* s) {
     console_write(s, strlen(s));
}

void console_cursor(uint8_t r, uint8_t c) {
     if (r >= CONSOLE_HEIGHT || c >= CONSOLE_WIDTH) {
          panicf("illegal cursor location %u, %u", r, c);
     }

     row = r;
     col = c;

     uint16_t base_vga_port = *(uint16_t *) 0x463; // read base vga port from bios data

     outb(base_vga_port, 0x0e);
     outb(base_vga_port + 1, ((row * CONSOLE_WIDTH + col) >> 8) & 0xff);

     outb(base_vga_port, 0x0f);
     outb(base_vga_port + 1, (row * CONSOLE_WIDTH + col) & 0xff);
}

void kprintf(char* fmt, ...) {
     va_list va;
     va_start(va, fmt);
     char buff[BUFFSIZE];
     vsprintf(buff, fmt, va);
     va_end(va);

     console_puts(buff);
}
