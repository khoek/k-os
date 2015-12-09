#include <stdio.h>
#include <string.h>
#include <unistd.h>

FILE stdin_file = {.fd = 0};
FILE stdout_file = {.fd = 1};
FILE stderr_file = {.fd = 2};

int printf(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int len = vfprintf(stdout, fmt, va);
    va_end(va);

    return len;
}

int fprintf(FILE *file, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int len = vfprintf(file, fmt, va);
    va_end(va);

    return len;
}

char buff[2048];
int vfprintf(FILE *file, const char *fmt, va_list args) {
    int len = vsprintf(buff, fmt, args);
    return write(file->fd, buff, len);
}
