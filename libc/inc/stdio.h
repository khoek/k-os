#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif
