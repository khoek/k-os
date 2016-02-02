#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

#include <stddef.h>
#include <inttypes.h>

typedef int32_t intptr_t;

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

int execve(const char *filename, char *const argv[], char *const envp[]);
pid_t fork();

int close(int fildes);

int brk(void *addr);
void * sbrk(intptr_t incr);

#endif
