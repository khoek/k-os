#include "lib/string.h"
#include "lib/printf.h"
#include "common/compiler.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "arch/registers.h"
#include "time/clock.h"
#include "driver/console/console.h"

#define LOGBUFFSIZE 32768
#define LINEBUFFSIZE 1024

static uint32_t front;
static char log_buff[LOGBUFFSIZE];
static DEFINE_SPINLOCK(log_lock);

static char line_buff[LINEBUFFSIZE];
static DEFINE_SPINLOCK(line_buff_lock);

void kprint(const char *str) {
    uint32_t time = uptime();

    uint32_t flags;
    spin_lock_irqsave(&log_lock, &flags);

    int len = sprintf(log_buff + front, "[%5u.%03u] %s\n", time / MILLIS_PER_SEC, time % MILLIS_PER_SEC, str, "\n");

    if(con_global) {
        vram_puts(con_global, log_buff + front);
    }

    front += len;

    spin_unlock_irqstore(&log_lock, flags);
}

void kprintf(const char *fmt, ...) {
    uint32_t flags;
    spin_lock_irqsave(&line_buff_lock, &flags);

    va_list va;
    va_start(va, fmt);
    vsprintf(line_buff, fmt, va);
    va_end(va);

    kprint(line_buff);

    spin_unlock_irqstore(&line_buff_lock, flags);
}
