#ifndef KERNEL_FS_TYPE_DEVFS_H
#define KERNEL_FS_TYPE_DEVFS_H

#include "fs/char.h"
#include "fs/block.h"

char * devfs_get_strpath(char *name);

void devfs_publish_pending();

void devfs_add_chardev(char_device_t *device, char *name);
void devfs_add_blockdev(block_device_t *device, char *name);

#endif
