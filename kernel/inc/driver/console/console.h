#ifndef KERNEL_VIDEO_CONSOLE_H
#define KERNEL_VIDEO_CONSOLE_H

#include "lib/int.h"
#include "device/device.h"
#include "fs/char.h"

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

#define B_KEY      0x30
#define C_KEY      0x2e
#define S_KEY      0x1f

#define CAPS_KEY   0x3a
#define LSHIFT_KEY 0x2a
#define RSHIFT_KEY 0x36
#define CTRL_KEY   0x1d
#define ALT_KEY    0x38
#define SYSRQ_KEY  0x54

typedef struct console {
    device_t device;
    char_device_t *chardev;

    uint16_t port;
    char *vram;

    uint32_t row, col;
    char color;

    spinlock_t lock;
} console_t;

extern console_t *con_global;

void vram_init(console_t *console);
void keyboard_init(console_t *console);

ssize_t keybuff_read(char* buff, size_t len);

void vram_color(console_t *con, char c);
void vram_clear(console_t *con);
void vram_write(console_t *con, const char *str, size_t len);
void vram_cursor(console_t *con, uint8_t r, uint8_t c);
void vram_puts(console_t *con, const char* str);
void vram_putsf(console_t *con, const char* str, ...);

#endif
