#ifndef LIBC_K_SYS_H
#define LIBC_K_SYS_H

#define PAGE_SIZE 4096

extern void *_data_start;
extern void *_data_end;

#include "syscall.h"
#include "sys/types.h"

void _msleep(uint32_t millis);

#endif
