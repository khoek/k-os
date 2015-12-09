#ifndef KERNEL_VIDEO_CONSOLE_H
#define KERNEL_VIDEO_CONSOLE_H

#include "lib/int.h"
#include "device/device.h"

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

typedef struct console {
    device_t device;

    uint16_t port;
    char *vram;

    uint32_t row, col;
    char color;
    spinlock_t lock;
} console_t;

console_t * console_primary();

void console_color(console_t *con, const char c);
void console_clear(console_t *con);
void console_write(console_t *con, const char *str, const uint8_t len);
void console_cursor(console_t *con, const uint8_t r, const uint8_t c);
void console_puts(console_t *con, const char* str);
void console_putsf(console_t *con, const char* str, ...);

#endif
