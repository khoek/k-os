#include <stdarg.h>

#include "lib/string.h"
#include "lib/printf.h"
#include "common/asm.h"
#include "bug/debug.h"
#include "arch/bios.h"
#include "sync/spinlock.h"
#include "mm/mm.h"
#include "driver/console/console.h"

#define TABSTOP_WIDTH 8

#define PUTSF_BUFF_SIZE 1024

#define ESCSEQ_BUFF_SIZE 128

#define ASCII_BELL      '\x7'
#define ASCII_BACKSPACE '\x8'
#define ASCII_TAB       '\x9'

#define PORTOFF_ADDR 0
#define PORTOFF_DATA 1

#define REG_CURSOR_START 0x0A
#define REG_CURSOR_END   0x0B
#define REG_CURSOR_HI    0x0E
#define REG_CURSOR_LO    0x0F

static inline void _select_reg(console_t *con, uint8_t reg) {
    outb(con->port + PORTOFF_ADDR, reg);
}

static inline uint8_t _get_val(console_t *con) {
    return inb(con->port + PORTOFF_DATA);
}

static inline void _put_val(console_t *con, uint8_t val) {
    outb(con->port + PORTOFF_DATA, val);
}

static inline void set_reg(console_t *con, uint8_t reg, uint8_t val) {
    _select_reg(con, reg);
    _put_val(con, val);
}

//sets the bits indicated by mask of VGA register reg to (val & mask)
static inline void set_reg_bits(console_t *con, uint8_t reg, uint8_t mask,
    uint8_t val) {
    _select_reg(con, reg);
    uint8_t prev_val = 0;
    if(mask != 0xFF) {
        prev_val = _get_val(con);
    }
    _put_val(con, (prev_val & ~mask) | (val & mask));
}

#define _CURSOR_BLINK_ENABLE (1 << 6)
#define _CURSOR_SPEED_SLOW (0 << 5)
#define _CURSOR_SPEED_FAST (1 << 5)

//valid options for "blink" below:
#define CURSOR_BLINK_OFF  (0)
#define CURSOR_BLINK_SLOW ((_CURSOR_BLINK_ENABLE | _CURSOR_SPEED_SLOW))
#define CURSOR_BLINK_FAST ((_CURSOR_BLINK_ENABLE | _CURSOR_SPEED_SLOW))

//Specifies the top and bottom scanlines for the drawing of the cursor (0-15),
//along with timing and blink information.
static void cursor_enable(console_t *con, uint8_t top_scanline, uint8_t bot_scanline, uint8_t blink) {
    set_reg_bits(con, REG_CURSOR_START, 0x7F, (top_scanline & 0x1F)
                                            | (blink        & 0x60));
    set_reg_bits(con, REG_CURSOR_END  , 0x1F, (bot_scanline & 0x1F));
}

static void do_refresh_cursor(console_t *con) {
    uint16_t pos = con->row * CONSOLE_WIDTH + con->col;
    set_reg_bits(con, REG_CURSOR_LO, 0xFF, (uint8_t) (pos & 0xFF));
    set_reg_bits(con, REG_CURSOR_HI, 0x3F, (uint8_t) ((pos >> 8) & 0xFF));
}

static void do_clear(console_t *con) {
    con->row = 0;
    con->col = 0;
    for(uint32_t i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        con->vram[i * 2] = ' ';
        con->vram[i * 2 + 1] = con->color;
    }
    do_refresh_cursor(con);
}

static void do_erase(console_t *con, bool to_end) {
    int32_t dir = to_end ? 1 : -1;
    int32_t rstart = con->row;
    int32_t cstart = con->col;

    while(rstart >= 0 && rstart < CONSOLE_HEIGHT) {
        while(cstart >= 0 && cstart < CONSOLE_WIDTH) {
            con->vram[(rstart * CONSOLE_WIDTH + cstart) * 2] = ' ';
            con->vram[(rstart * CONSOLE_WIDTH + cstart) * 2 + 1] = con->color;
            cstart += dir;
        }
        cstart = dir == 1 ? 0 : CONSOLE_WIDTH - 1;
        rstart += dir;
    }
}

static void cursor_up(console_t *con, uint32_t amt) {
    con->row -= MIN(con->row, amt);
    do_refresh_cursor(con);
}

static void cursor_down(console_t *con, uint32_t amt) {
    con->row += MAX(CONSOLE_HEIGHT - con->row, amt);
    do_refresh_cursor(con);
}

static void cursor_forward(console_t *con, uint32_t amt) {
    con->col += MAX(CONSOLE_WIDTH - con->col, amt);
    do_refresh_cursor(con);
}

static void cursor_back(console_t *con, uint32_t amt) {
    con->col -= MIN(con->col, amt);
    do_refresh_cursor(con);
}

static void cursor_position(console_t *con, uint32_t r, uint32_t c) {
    if(r > 0) r--;
    if(c > 0) c--;

    con->row = MIN(r, CONSOLE_HEIGHT - 1);
    con->col = MIN(c, CONSOLE_WIDTH - 1);

    do_refresh_cursor(con);
}

void vram_color(console_t *con, char c) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    con->color = c;

    spin_unlock_irqstore(&con->lock, flags);
}

void vram_clear(console_t *con) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    do_clear(con);

    spin_unlock_irqstore(&con->lock, flags);
}

//handle column and row overflows
static void fix_pos_overflow(console_t *con) {
    if(con->col == CONSOLE_WIDTH) {
        con->col = 0;
        con->row++;
    }

    if(con->row == CONSOLE_HEIGHT) {
        memmove(con->vram, con->vram + CONSOLE_WIDTH * 2, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2);

        for(int i = 0; i < CONSOLE_WIDTH; i++) {
            (con->vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2] = ' ';
            (con->vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2 + 1] = con->color;
        }

        con->row = CONSOLE_HEIGHT - 1;
    }
}

//FIXME define system for registering this
extern void beep();

static void process_char(console_t *con, char c) {
    switch(c) {
        //Bell
        case ASCII_BELL: {
            beep();
            break;
        }
        //Backspace
        case ASCII_BACKSPACE: {
            cursor_back(con, 1);
            break;
        }
        //Tab
        case ASCII_TAB: {
            con->col++;
            if(con->col == CONSOLE_WIDTH) {
                fix_pos_overflow(con);
                break;
            }

            while((con->col % TABSTOP_WIDTH) && con->col < CONSOLE_WIDTH - 1) {
                con->col++;
            }
            break;
        }
        case '\n': {
            con->col = 0;
            con->row++;
            fix_pos_overflow(con);
            break;
        }
        case '\r': {
            con->col = 0;
            break;
        }
        default: {
            con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2] = c;
            con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2 + 1] = con->color;
            con->col++;
            fix_pos_overflow(con);
            break;
        }
    }
}

static void vram_putc(console_t *con, char c) {
    check_irqs_disabled();

    process_char(con, c);
}

static size_t read_escseq_args(console_t *con, int32_t *n, int32_t *m, char *c) {
    char *buff = con->escseq_buff;
    size_t max_len = con->escseq_buff_front;

    uint32_t off = 0;
    int32_t tmp = -1;
    int32_t *v = n;
    char cmd = 0;
    while(off < max_len) {
        char d = buff[off++];
        if(isdigit(d)) {
            if(tmp == -1) {
                tmp = 0;
            }

            tmp *= 10;
            tmp += d - '0';
        } else {
            if(tmp != -1) {
                *v = tmp;
            }

            if(d == ';' && v == n) {
                v = m;
                tmp = -1;
            } else {
                cmd = d;
                break;
            }
        }
    }

    if(cmd) {
        *c = cmd;
    }

    return off;
}

static void execute_escseq(console_t *con, char c, int32_t n, int32_t m) {
    if(!c) return;

    switch(c) {
        //Cursor Up
        case 'A': {
            cursor_up(con, n == -1 ? 1 : n);
            break;
        }
        //Cursor Down
        case 'B': {
            cursor_down(con, n == -1 ? 1 : n);
            break;
        }
        //Cursor Forward
        case 'C': {
            cursor_forward(con, n == -1 ? 1 : n);
            break;
        }
        //Cursor Back
        case 'D': {
            cursor_back(con, n == -1 ? 1 : n);
            break;
        }
        //Cursor Position
        case 'H': {
            cursor_position(con, n == -1 ? 0 : n, m == -1 ? 0 : m);
            break;
        }
        //Erase Display
        case 'J':{
            uint32_t v = n == -1 ? 0 : n;
            switch(v) {
                //clear to end of screen
                case 1: {
                    do_erase(con, false);
                    break;
                }
                //clear entire screen
                case 2: {
                    do_clear(con);
                    break;
                }
                //clear to start of screen
                case 0:
                default: {
                    do_erase(con, true);
                    break;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

static void interpret_escseq(console_t *con) {
    int32_t n = -1, m = -1;
    char c = 0;

    read_escseq_args(con, &n, &m, &c);
    execute_escseq(con, c, n, m);
}

static PURE inline bool is_terminating_char(char c) {
    return (c >= '@') && (c <= '~');
}

static void put_escseq_buff(console_t *con, char c) {
    if(!con->escseq_buff) {
        panic("escseq too early!");
    }

    con->escseq_buff[con->escseq_buff_front++] = c;

    if(is_terminating_char(c)) {
        interpret_escseq(con);
    }

    if(is_terminating_char(c) || con->escseq_buff_front == ESCSEQ_BUFF_SIZE) {
        con->escseq_buff_front = 0;
        con->state = TERM_NORMAL;
    }
}

static void vram_handle(console_t *con, char c) {
    switch(con->state) {
        case TERM_NORMAL: {
            switch(c) {
                case CSI_SINGLE: {
                    con->state = TERM_ESCSEQ;
                    break;
                }
                case CSI_DOUBLE_PARTA: {
                    con->state = TERM_EXPECT_CSI_PARTB;
                    break;
                }
                default: {
                    vram_putc(con, c);
                    break;
                }
            }
            break;
        }
        case TERM_EXPECT_CSI_PARTB: {
            switch(c) {
                case CSI_DOUBLE_PARTB: {
                    con->state = TERM_ESCSEQ;
                    break;
                }
                default: {
                    con->state = TERM_NORMAL;
                    vram_putc(con, CSI_DOUBLE_PARTA);
                    vram_putc(con, c);
                    break;
                }
            }
            break;
        }
        case TERM_ESCSEQ: {
            put_escseq_buff(con, c);
            break;
        }
        default: {
            BUG();
        }
    }
}

void vram_write(console_t *con, const char *str, size_t len) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    for(size_t i = 0; i < len; i++) {
        vram_handle(con, str[i]);
    }

    spin_unlock_irqstore(&con->lock, flags);

    do_refresh_cursor(con);
}

void vram_cursor(console_t *con, uint8_t r, uint8_t c) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    do_refresh_cursor(con);

    spin_unlock_irqstore(&con->lock, flags);
}

void vram_puts(console_t *con, const char* str) {
    vram_write(con, str, strlen(str));
}

//FIXME remove the need for this
static char buff[PUTSF_BUFF_SIZE];
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

void __init vram_early_init(console_t *console) {
    console->row = 0;
    console->col = 0;
    console->color = 0x07;
    console->state = TERM_NORMAL;
    console->escseq_buff = NULL;
    console->escseq_buff_front = 0;
    console->vram = (void *) BIOS_VRAM;
    console->port = bda_getw(BDA_VRAM_PORT);

    cursor_enable(console, 14, 15, CURSOR_BLINK_SLOW);

    vram_clear(con_global);
}

void __init vram_early_remap(console_t *console) {
    console->vram = map_page(BIOS_VRAM);
}

void vram_init(console_t *console) {
    console->escseq_buff = kmalloc(ESCSEQ_BUFF_SIZE);
    console->escseq_buff_front = 0;
}
