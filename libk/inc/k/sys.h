#ifndef LIBK_K_SYS_H
#define LIBK_K_SYS_H

#define PAGE_SIZE 4096

extern void *_data_start;
extern void *_data_end;

#include "k/types.h"
#include "syscall.h"

void _msleep(uint32_t millis);

#endif
