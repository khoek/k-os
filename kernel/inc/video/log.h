#ifndef KERNEL_VIDEO_LOG_H
#define KERNEL_VIDEO_LOG_H

#include "sync/spinlock.h"

DECLARE_SPINLOCK(log_lock);

void log(const char *str);
void logf(const char *fmt, ...);

#endif
