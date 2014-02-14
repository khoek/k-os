#include "lib/string.h"
#include "lib/printf.h"
#include "common/compiler.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "arch/registers.h"
#include "time/clock.h"
#include "video/console.h"

#define BUFFSIZE 1024

DEFINE_SPINLOCK(log_lock);

static char buff[BUFFSIZE];
static DEFINE_SPINLOCK(buff_lock);

static void print_time() {
    uint32_t time = uptime();
    console_putsf("[%5u.%03u] ", time / MILLIS_PER_SEC, time % MILLIS_PER_SEC);
}

void log(const char *str) {
    uint32_t flags;
    spin_lock_irqsave(&log_lock, &flags);

    print_time();

    console_puts(str);
    console_puts("\n");

    spin_unlock_irqstore(&log_lock, flags);
}

void logf(const char *fmt, ...) {
    uint32_t flags;
    spin_lock_irqsave(&buff_lock, &flags);

    va_list va;
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);

    log(buff);

    spin_unlock_irqstore(&buff_lock, flags);
}
