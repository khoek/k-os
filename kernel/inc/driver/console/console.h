#ifndef KERNEL_DRIVER_CONSOLE_CONSOLE_H
#define KERNEL_DRIVER_CONSOLE_CONSOLE_H

#include "common/types.h"
#include "init/initcall.h"
#include "device/device.h"
#include "fs/char.h"

#define RELEASE_BIT (1 << 7)

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

#define CSI_SINGLE ((char) 0x9B)
#define CSI_DOUBLE_PARTA ((char) 0x1B)
#define CSI_DOUBLE_PARTB ((char) '[')
#define CSI_DOUBLE "\x1b["

#define B_KEY      0x30
#define C_KEY      0x2e
#define S_KEY      0x1f

#define CAPS_KEY   0x3a
#define LSHIFT_KEY 0x2a
#define RSHIFT_KEY 0x36
#define CTRL_KEY   0x1d
#define ALT_KEY    0x38
#define SYSRQ_KEY  0x54

typedef enum {
    TERM_NORMAL,
    TERM_ESCSEQ,
    TERM_EXPECT_CSI_PARTB,
} term_state_t;

typedef struct console {
    device_t device;
    char_device_t *chardev;

    uint16_t port;
    char *vram;

    uint32_t row, col;
    char color;

    term_state_t state;

    char *escseq_buff;
    size_t escseq_buff_front;

    bool lockup;
    spinlock_t lock;
} console_t;

extern console_t *con_global;

void __init console_early_init();
void __init console_early_remap();

void vram_init(console_t *console);
void keyboard_init(console_t *console);

ssize_t keybuff_read(uint8_t *buff, size_t len);
bool keybuff_is_empty();
void keyboard_poll();

void vram_color(console_t *con, char c);
void vram_clear(console_t *con);
void vram_write(console_t *con, const char *str, size_t len);
void vram_cursor(console_t *con, uint8_t r, uint8_t c);
void vram_puts(console_t *con, const char* str);
void vram_putsf(console_t *con, const char* str, ...);

void console_lockup(console_t *con);

#endif
