#ifndef KERNEL_FS_FD_H
#define KERNEL_FS_FD_H

#include "common/types.h"
#include "fs/vfs.h"

#define FD_INVALID (-1)

file_t * gfdt_obtain();

void gfdt_get(file_t *gfd);
void gfdt_put(file_t *gfd);

#endif
