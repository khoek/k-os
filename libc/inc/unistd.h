#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

#include <inttypes.h>

typedef int32_t intptr_t;

int close(int fildes);

int brk(void *addr);
void * sbrk(intptr_t incr);

#endif
