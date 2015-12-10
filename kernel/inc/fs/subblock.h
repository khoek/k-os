#ifndef KERNEL_FS_SUBBLOCK_H
#define KERNEL_FS_SUBBLOCK_H

#include "lib/int.h"
#include "common/list.h"

block_device_t * subblock_device_open(block_device_t *device, uint32_t start, uint32_t size);

#endif
