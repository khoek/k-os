#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include <stdint.h>

#define CONSOLE_WIDTH	80
#define CONSOLE_HEIGHT	25

uint8_t console_row();
uint8_t console_col();

void console_color(const char c);
void console_clear();
void console_write(const char *str, const uint8_t len);
void console_cursor(const uint8_t r, const uint8_t c);
void console_puts(const char* str);
void console_putsf(const char* fmt, ...);

#endif
