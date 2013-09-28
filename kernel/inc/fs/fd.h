#ifndef KERNEL_FS_FD_H
#define KERNEL_FS_FD_H

#include "lib/int.h"

typedef int32_t gfd_idx_t;
typedef int32_t ufd_idx_t;

typedef struct gfd gfd_t;
typedef struct ufd ufd_t;

#include "fs/vfs.h"

#define FD_INVALID (-1)

struct gfd {
    uint32_t refs;
    file_t *file;
};

struct ufd {
    uint32_t refs;
    uint32_t flags;
    gfd_idx_t gfd;
};

gfd_idx_t gfdt_add(file_t *file);

file_t * gfdt_get(gfd_idx_t gfd_idx);
void gfdt_put(gfd_idx_t gfd_idx);

#endif
