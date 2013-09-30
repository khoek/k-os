#ifndef KERNEL_FS_TYPE_DEVFS_H
#define KERNEL_FS_TYPE_DEVFS_H

#include "fs/block.h"

void devfs_add_device(block_device_t *device, char *name);

#endif
