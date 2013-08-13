#ifndef KERNEL_FS_VFS_H
#define KERNEL_FS_VFS_H

#define FILE_FLAG_STATIC (1 << 0)

#include "lib/int.h"
#include "common/list.h"

typedef struct file file_t;
typedef struct dentry dentry_t;

typedef struct file_ops {
    ssize_t (*read)(file_t *file, void *buff, size_t bytes);
    ssize_t (*write)(file_t *file, void *buff, size_t bytes);
} file_ops_t;

struct file {
    uint32_t flags;
    void *private;

    file_ops_t *ops;
};

struct dentry {
    char *filename;
    file_t *file;

    dentry_t *parent;
    list_head_t children;

    list_head_t list;
};

typedef struct vfs_provider {
    list_head_t list;
} vfs_provider_t;

#endif
