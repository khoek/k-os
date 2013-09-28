#ifndef KERNEL_FS_VFS_H
#define KERNEL_FS_VFS_H

#define PATH_SEPARATOR '/'

#define FSTYPE_FLAG_NODEV (1 << 0)

#define FS_FLAG_STATIC (1 << 0)
#define FS_FLAG_NODEV  (1 << 1)

typedef struct block_device block_device_t;
typedef struct block_device_ops block_device_ops_t;

typedef struct disk_label disk_label_t;

typedef struct fs_type fs_type_t;
typedef struct fs fs_t;

typedef struct file file_t;
typedef struct file_ops file_ops_t;

typedef struct inode inode_t;
typedef struct inode_ops inode_ops_t;

typedef struct dentry dentry_t;

#include "lib/int.h"
#include "common/list.h"
#include "common/hashtable.h"
#include "fs/fd.h"

#define DENTRY_HASH_BITS 5

struct block_device {
    fs_t *fs;

    size_t size;
    size_t block_size;
    block_device_ops_t *ops;
};

struct block_device_ops {
    ssize_t (*read)(block_device_t * device, void *buff, size_t block, size_t count);
    ssize_t (*write)(block_device_t * device, void *buff, size_t block, size_t count);
};

struct disk_label {
    bool (*probe)(block_device_t *device);

    list_head_t list;
};

struct fs_type {
    char *name;
    uint32_t flags;

    fs_t * (*open)(block_device_t *device);

    hashtable_node_t node;
};

struct fs {
    dentry_t *root;
    dentry_t *mountpoint;
    fs_type_t *type;
};

struct file {
    dentry_t *dentry;

    void *private;

    file_ops_t *ops;
};

struct file_ops {
    void (*open)(file_t *file, inode_t *inode);
    ssize_t (*seek)(file_t *file, size_t bytes);
    ssize_t (*read)(file_t *file, size_t bytes);
    void (*close)(file_t *file);
};

struct inode {
    fs_t *fs;
    inode_ops_t *ops;
};

struct inode_ops {
    file_ops_t *file_ops;

    void (*lookup)(inode_t *inode, dentry_t *target);
    dentry_t * (*mkdir)(inode_t *inode, char *name);
};

struct dentry {
    char *name;

    inode_t *inode;

    dentry_t *parent;
    DECLARE_HASHTABLE(children, DENTRY_HASH_BITS);
    list_head_t siblings;

    list_head_t list;
    hashtable_node_t node;
};

file_t * file_alloc(file_ops_t *ops);

void register_block_device(block_device_t *device, char *name);
void register_disk(block_device_t *device);
void register_disk_label(disk_label_t *disk_label);
void register_fs_type(fs_type_t *fs_type);

bool vfs_mount(block_device_t *device, const char *type, dentry_t *mountpoint);
bool vfs_umount(dentry_t *d);

dentry_t * vfs_lookup(dentry_t *d, const char *path);
gfd_idx_t vfs_open_file(inode_t *inode);

#endif
