#ifndef KERNEL_FS_ROOTRAMFS_H
#define KERNEL_FS_ROOTRAMFS_H

#include "common/types.h"

void rootramfs_load(void *start, uint32_t len);

#endif
