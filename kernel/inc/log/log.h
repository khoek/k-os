#ifndef KERNEL_LOG_LOG_H
#define KERNEL_LOG_LOG_H

#include "sync/spinlock.h"

DECLARE_SPINLOCK(log_lock);

void kprint(const char *str);
void kprintf(const char *fmt, ...);

void vlog_disable();
void vlog_enable();

#endif
