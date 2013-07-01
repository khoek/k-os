#include "string.h"
#include "printf.h"
#include "common.h"
#include "clock.h"
#include "console.h"
#include "panic.h"
#include "registers.h"
#include "spinlock.h"

#define BUFFSIZE 1024

static char buff[BUFFSIZE];

static SPINLOCK_INIT(buff_lock);
static SPINLOCK_INIT(log_lock);

static void print_time() {
    uint32_t time = uptime();
    console_putsf("[%5u.%03u] ", time / MSEC_IN_SEC, time % MSEC_IN_SEC);
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
