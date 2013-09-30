#include "lib/int.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "mm/cache.h"
#include "fs/vfs.h"
#include "video/log.h"

static void ramfs_file_open(file_t *file, inode_t *inode) {
}

static void ramfs_file_close(file_t *file) {
}

static file_ops_t ramfs_file_ops = {
    .open   = ramfs_file_open,
    .close  = ramfs_file_close,
};

static void ramfs_inode_lookup(inode_t *inode, dentry_t *dentry) {
    dentry->inode = inode_alloc(inode->fs, inode->ops);
    dentry->inode->mode = 0755;
}

static dentry_t * ramfs_inode_mkdir(inode_t *inode, char *name) {
    return NULL;
}

static inode_ops_t ramfs_inode_ops = {
    .file_ops = &ramfs_file_ops,

    .lookup = ramfs_inode_lookup,
    .mkdir  = ramfs_inode_mkdir,
};

static fs_t * ramfs_open(block_device_t *device);

static fs_type_t ramfs = {
    .name  = "ramfs",
    .flags = FSTYPE_FLAG_NODEV,
    .open  = ramfs_open,
};

static fs_t * ramfs_open(block_device_t *device) {
    dentry_t *root = dentry_alloc("");
    dentry_add_child(dentry_alloc("bin"), root);
    dentry_add_child(dentry_alloc("dev"), root);

    fs_t *fs = fs_alloc(&ramfs, device, root);

    root->inode = inode_alloc(fs, &ramfs_inode_ops);
    root->inode->mode = 0755;

    return fs;
}

static INITCALL ramfs_register() {
    register_fs_type(&ramfs);

    return 0;
}

core_initcall(ramfs_register);
