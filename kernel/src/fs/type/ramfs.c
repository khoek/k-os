#include "lib/int.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "mm/cache.h"
#include "fs/vfs.h"
#include "log/log.h"

static void ramfs_file_open(file_t *file, inode_t *inode) {
}

static void ramfs_file_close(file_t *file) {
}

static file_ops_t ramfs_file_ops = {
    .open   = ramfs_file_open,
    .close  = ramfs_file_close,
};

static void ramfs_inode_lookup(inode_t *inode, dentry_t *dentry);
static dentry_t * ramfs_inode_mkdir(inode_t *inode, char *name, uint32_t mode);

static inode_ops_t ramfs_inode_ops = {
    .file_ops = &ramfs_file_ops,

    .lookup = ramfs_inode_lookup,
    .mkdir  = ramfs_inode_mkdir,
};

static void ramfs_inode_lookup(inode_t *inode, dentry_t *dentry) {
}

static dentry_t * ramfs_inode_mkdir(inode_t *inode, char *name, uint32_t mode) {
    dentry_t *new = dentry_alloc(name);
    new->inode = inode_alloc(inode->fs, &ramfs_inode_ops);
    new->inode->flags |= INODE_FLAG_DIRECTORY;
    new->inode->mode = mode;

    return new;
}

static dentry_t * ramfs_create(fs_type_t *type, const char *device);

static fs_type_t ramfs = {
    .name  = "ramfs",
    .flags = FSTYPE_FLAG_NODEV,
    .create  = ramfs_create,
};

static void ramfs_fill(fs_t *fs) {
    dentry_t *root = fs->root = dentry_alloc("");
    root->fs = fs;

    root->inode = inode_alloc(fs, &ramfs_inode_ops);
    root->inode->flags |= INODE_FLAG_DIRECTORY;
    root->inode->mode = 0755;
}

static dentry_t * ramfs_create(fs_type_t *type, const char *device) {
    return fs_create_nodev(type, ramfs_fill);
}

static INITCALL ramfs_register() {
    register_fs_type(&ramfs);

    return 0;
}

core_initcall(ramfs_register);
