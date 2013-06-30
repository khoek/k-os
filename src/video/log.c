#include "string.h"
#include "printf.h"
#include "common.h"
#include "clock.h"
#include "console.h"
#include "panic.h"
#include "registers.h"

#define BUFFSIZE 1024
static char time_buff[BUFFSIZE];
static void print_time() {
    uint32_t time = uptime();
    
    sprintf(time_buff, "[%5u.%03u] ", time / MSEC_IN_SEC, time % MSEC_IN_SEC);
    
    console_puts(time_buff);
}

void log(const char *str) {
    print_time();

    console_puts(str);
    console_puts("\n");
}

static char log_buff[BUFFSIZE];
void logf(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vsprintf(log_buff, fmt, va);
    va_end(va);

    log(log_buff);
}
