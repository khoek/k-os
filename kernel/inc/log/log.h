#ifndef KERNEL_LOG_LOG_H
#define KERNEL_LOG_LOG_H

#include "sync/spinlock.h"

void kprint(const char *str);
void kprintf(const char *fmt, ...);

void vlog_disable();
void vlog_enable();

#endif
