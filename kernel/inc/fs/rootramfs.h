#ifndef KERNEL_FS_ROOTRAMFS_H
#define KERNEL_FS_ROOTRAMFS_H

#include "lib/int.h"

void rootramfs_load(void *start, uint32_t len);

#endif
