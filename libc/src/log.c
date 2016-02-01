#include <string.h>
#include <stdio.h>
#include <k/sys.h>
#include <k/log.h>

#define KPRINTF_BUFF_SIZE 1024
static char buff[KPRINTF_BUFF_SIZE];

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
