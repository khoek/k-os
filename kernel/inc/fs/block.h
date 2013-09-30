#ifndef KERNEL_FS_BLOCK_H
#define KERNEL_FS_BLOCK_H

typedef struct block_device block_device_t;
typedef struct block_device_ops block_device_ops_t;

#include "lib/int.h"
#include "fs/vfs.h"

struct block_device {
    fs_t *fs;
    dentry_t *dentry;

    void *private;

    size_t size;
    size_t block_size;
    block_device_ops_t *ops;
};

struct block_device_ops {
    ssize_t (*read)(block_device_t *device, void *buff, size_t block, size_t count);
    ssize_t (*write)(block_device_t *device, void *buff, size_t block, size_t count);
};

block_device_t * block_device_alloc();
void register_block_device(block_device_t *device, char *name);

#endif
