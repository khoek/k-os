#include "string.h"
#include "printf.h"
#include "pit.h"
#include "console.h"

#define BUFFSIZE 1024
static char buff[BUFFSIZE];

static void print_time() {
    uint32_t time = uptime();
    sprintf(buff, "[%5u.%02u] ", time / 100, time % 100);
    console_puts(buff);
}

void log(const char *str) {
    print_time();

    console_puts(str);
    console_puts("\n");
}

void logf(const char *fmt, ...) {
    print_time();

    va_list va;
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);

    console_puts(buff);
    console_puts("\n");
}
