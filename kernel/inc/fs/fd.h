#ifndef KERNEL_FS_FD_H
#define KERNEL_FS_FD_H

#include "lib/int.h"

typedef struct gfd gfd_t;

typedef struct fd_ops {
    void (*close)(gfd_t *);
} fd_ops_t;

struct gfd {
    uint32_t flags;
    fd_ops_t *ops;
    void *private;
};

typedef struct ufd {
    uint32_t flags;
    uint32_t gfd;
} ufd_t;

typedef uint32_t gfd_idx_t;
typedef uint32_t ufd_idx_t;

gfd_idx_t gfdt_add(uint32_t flags, fd_ops_t *ops, void *private);
void gfdt_rm(gfd_idx_t gfd_idx);

#endif
