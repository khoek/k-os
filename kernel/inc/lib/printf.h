#ifndef KERNEL_LIB_PRINTF_H
#define KERNEL_LIB_PRINTF_H

#include <stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif
