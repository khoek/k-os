#ifndef KERNEL_FS_CHAR_H
#define KERNEL_FS_CHAR_H

typedef struct char_device char_device_t;
typedef struct char_device_ops char_device_ops_t;

#include "common/types.h"
#include "sync/spinlock.h"
#include "fs/vfs.h"

struct char_device_ops {
    ssize_t (*read)(char_device_t *device, char *buff, size_t len);
    ssize_t (*write)(char_device_t *device, char *buff, size_t len);
};

struct char_device {
    void *private;

    char_device_ops_t *ops;

    spinlock_t lock;
};

char_device_t * char_device_alloc();
void register_char_device(char_device_t *device, char *name);

#endif
