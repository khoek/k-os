#include <stdint.h>

#define CONSOLE_WIDTH	80
#define CONSOLE_HEIGHT	25

uint8_t console_row();
uint8_t console_col();

void console_color(char c);
void console_clear();
void console_write(char *s, uint8_t len);
void console_puts(char* s);
void console_cursor(uint8_t r, uint8_t c);

void kprintf(char* fmt, ...);
