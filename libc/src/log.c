#include <string.h>
#include <stdio.h>
#include <k/sys.h>
#include <k/log.h>

#define BUFFSIZE 1024
static char buff[BUFFSIZE];

void kprint(const char *str) {
    _log(str, strlen(str));
}

void kprintf(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);

    kprint(buff);
}
