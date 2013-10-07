#ifndef KERNEL_FS_VFS_H
#define KERNEL_FS_VFS_H

#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"

#define FSTYPE_FLAG_NODEV (1 << 0)

#define FS_FLAG_STATIC (1 << 0)
#define FS_FLAG_NODEV  (1 << 1)

#define INODE_FLAG_DIRECTORY  (1 << 0)
#define INODE_FLAG_MOUNTPOINT (1 << 1)

typedef struct fs_type fs_type_t;
typedef struct fs fs_t;

typedef struct mount mount_t;

typedef struct path path_t;

typedef struct file file_t;
typedef struct file_ops file_ops_t;

typedef struct inode inode_t;
typedef struct inode_ops inode_ops_t;

typedef struct dentry dentry_t;

typedef struct stat stat_t;

extern mount_t *root_mount;

#include "lib/int.h"
#include "common/list.h"
#include "common/hashtable.h"
#include "fs/fd.h"
#include "fs/block.h"

#define DENTRY_HASH_BITS 5

struct fs_type {
    const char *name;
    uint32_t flags;

    dentry_t * (*create)(fs_type_t *fs_type, const char *device);
    void (*destroy)(fs_t *fs);

    list_head_t instances;

    hashtable_node_t node;
};

struct fs {
    fs_type_t *type;
    void *private;
    dentry_t *root;

    list_head_t list;
};

struct mount {
    mount_t *parent;
    fs_t *fs;
    dentry_t *mountpoint;

    hashtable_node_t node;
};

struct path {
    mount_t *mount;
    dentry_t *dentry;
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

    uint32_t flags;

    uint32_t dev;
    uint32_t ino;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint32_t rdev;
    int32_t size;

    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;

    int32_t blkshift;
    int32_t blocks;

    union {
        block_device_t *block;
    } device;

    void *private;
};

struct inode_ops {
    file_ops_t *file_ops;

    void (*lookup)(inode_t *inode, dentry_t *target);
    dentry_t * (*mkdir)(inode_t *inode, char *name, uint32_t mode);
    void (*getattr)(dentry_t *dentry, stat_t *stat);
};

struct dentry {
    fs_t *fs;
    const char *name;
    uint32_t flags;

    inode_t *inode;

    dentry_t *parent;
    dentry_t *child;
    DECLARE_HASHTABLE(children, DENTRY_HASH_BITS);
    list_head_t siblings;

    void *private;

    hashtable_node_t node;
};

struct stat {
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t st_rdev;
    int32_t st_size;

    uint32_t st_atime;
    uint32_t st_mtime;
    uint32_t st_ctime;

    int32_t st_blksize;
    int32_t st_blocks;
};

dentry_t * dentry_alloc(const char *name);
inode_t * inode_alloc(fs_t *fs, inode_ops_t *ops);
file_t * file_alloc(file_ops_t *ops);

void dentry_add_child(dentry_t *child, dentry_t *parent);

void register_fs_type(fs_type_t *fs_type);

mount_t * vfs_mount(const char *raw_type, const char *device, path_t *mountpoint);
fs_t * vfs_fs_create(const char *raw_type, const char *device);
void vfs_fs_destroy(fs_t *fs);
mount_t * vfs_do_mount(fs_t *fs, path_t *mountpoint);
mount_t * vfs_mount_create(fs_t *fs);
bool vfs_mount_add(mount_t *mount, path_t *mountpoint);
void vfs_mount_destroy(mount_t *mount);

bool vfs_umount(path_t *mountpoint);

bool vfs_mkdir(path_t *start, const char *pathname, uint32_t mode);

dentry_t * fs_create_dev(fs_type_t *type, const char *device, void (*fill)(fs_t *fs, block_device_t *device));
dentry_t * fs_create_nodev(fs_type_t *type, void (*fill)(fs_t *fs));
dentry_t * fs_create_single(fs_type_t *type, void (*fill)(fs_t *fs));

void vfs_getattr(dentry_t *dentry, stat_t *stat);
void generic_getattr(inode_t *inode, stat_t *stat);

bool vfs_lookup(path_t *start, const char *path, path_t *out);
gfd_idx_t vfs_open_file(inode_t *inode);

#endif
