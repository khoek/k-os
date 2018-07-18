#ifndef KERNEL_FS_TYPE_DEVFS_H
#define KERNEL_FS_TYPE_DEVFS_H

#include "fs/vfs.h"
#include "fs/char.h"
#include "fs/block.h"

typedef enum device_type {
    CHAR_DEV,
    BLCK_DEV,
} device_type_t;

typedef struct devfs_device {
    list_head_t list;

    char *name;

    dentry_t *dentry;

    device_type_t type;
    union {
        void *dev;
        block_device_t *blockdev;
        char_device_t *chardev;
    };
} devfs_device_t;

static char_device_t * devfs_get_chardev(file_t *file) {
    devfs_device_t *device = file->private;
    return device->chardev;
}

static block_device_t * devfs_get_blockdev(file_t *file) {
    devfs_device_t *device = file->private;
    return device->blockdev;
}

void devfs_publish_pending();

void devfs_add_chardev(char_device_t *device, char *name);
void devfs_add_blockdev(block_device_t *device, char *name);

int32_t devfs_lookup(const char *path, path_t *out);

#endif
