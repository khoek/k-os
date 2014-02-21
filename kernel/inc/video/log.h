#ifndef KERNEL_VIDEO_LOG_H
#define KERNEL_VIDEO_LOG_H

#include "sync/spinlock.h"

DECLARE_SPINLOCK(log_lock);

void kprint(const char *str);
void kprintf(const char *fmt, ...);

#endif
