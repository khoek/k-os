#include <stdarg.h>

#include "lib/string.h"
#include "lib/printf.h"
#include "common/asm.h"
#include "bug/debug.h"
#include "arch/bios.h"
#include "sync/spinlock.h"
#include "mm/mm.h"
#include "driver/console/console.h"

#define PUTSF_BUFF_SIZE 1024

#define ESCSEQ_BUFF_SIZE 128

static void do_clear(console_t *con) {
    for(uint32_t i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        con->vram[i * 2] = ' ';
        con->vram[i * 2 + 1] = con->color;
    }
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

static void vram_putc(console_t *con, char c) {
    check_irqs_disabled();

    if(c == '\n') {
        con->col = 0;
        con->row++;
    } else if(c == '\r') {
        con->col = 0;
    } else {
        con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2] = c;
        con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2 + 1] = con->color;
        con->col++;
    }

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

static void do_refresh_cursor(console_t *con) {
    outb(con->port, 0x0e);
    outb(con->port + 1, ((con->row * CONSOLE_WIDTH + con->col) >> 8) & 0xff);

    outb(con->port, 0x0f);
    outb(con->port + 1, (con->row * CONSOLE_WIDTH + con->col) & 0xff);
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
            uint32_t amt = n == -1 ? 1 : n;
            con->row -= MIN(con->row, amt);
            do_refresh_cursor(con);
            break;
        }
        //Cursor Down
        case 'B': {
            uint32_t amt = n == -1 ? 1 : n;
            con->row += MAX(CONSOLE_HEIGHT - con->row, amt);
            do_refresh_cursor(con);
            break;
        }
        //Cursor Forward
        case 'C': {
            uint32_t amt = n == -1 ? 1 : n;
            con->col += MAX(CONSOLE_WIDTH - con->col, amt);
            do_refresh_cursor(con);
            break;
        }
        //Cursor Back
        case 'D': {
            uint32_t amt = n == -1 ? 1 : n;
            con->col -= MIN(con->col, amt);
            do_refresh_cursor(con);
            break;
        }
        //Cursor Position
        case 'H': {
            uint32_t nr = 0;
            if(n > 0) {
                nr = n - 1;
            }

            uint32_t nc = 0;
            if(m > 0) {
                nc = m - 1;
            }

            con->row = MIN(nr, CONSOLE_HEIGHT - 1);
            con->col = MIN(nc, CONSOLE_WIDTH - 1);

            do_refresh_cursor(con);
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

    vram_clear(con_global);
}

void __init vram_early_remap(console_t *console) {
    console->vram = map_page(BIOS_VRAM);
}

void vram_init(console_t *console) {
    console->escseq_buff = kmalloc(ESCSEQ_BUFF_SIZE);
    console->escseq_buff_front = 0;
}
