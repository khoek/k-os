#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdarg.h>

#define FILE struct file_data

struct file_data {
    int fd;
};

extern FILE stdin_file;
extern FILE stdout_file;
extern FILE stderr_file;

#define stdin &stdin_file
#define stdout &stdout_file
#define stderr &stderr_file

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

int printf(const char *fmt, ...);
int fprintf(FILE *file, const char *fmt, ...);
int vfprintf(FILE *file, const char *fmt, va_list args);

#endif
