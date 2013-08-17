#include "lib/int.h"
#include "common/init.h"
#include "mm/cache.h"
#include "fs/vfs.h"
#include "video/log.h"

inode_t * ramfs_inode_lookup(inode_t *inode, dentry_t *dentry) {
    return NULL;
}

file_t * ramfs_inode_open(inode_t *inode) {
    return NULL;
}

dentry_t * ramfs_inode_mkdir(inode_t *inode, char *name) {
    return NULL;
}

static inode_ops_t root_inode_ops = {
    .lookup = ramfs_inode_lookup,
    .open   = ramfs_inode_open,
    .mkdir  = ramfs_inode_mkdir,
};

static fs_t * ramfs_open(block_device_t *device) {
    fs_t *fs = kmalloc(sizeof(fs_t));

    dentry_t *root = kmalloc(sizeof(dentry_t));
    list_init(&root->children);
    list_init(&root->siblings);

    fs->root = root;

    inode_t *root_inode = kmalloc(sizeof(inode_t));
    root->inode = root_inode;

    root_inode->fs = fs;
    root_inode->ops = &root_inode_ops;

    return fs;
}

fs_type_t ramfs = {
    .name  = "ramfs",
    .flags = FSTYPE_FLAG_NODEV,
    .open  = ramfs_open,
};

static INITCALL ramfs_register() {
    register_fs_type(&ramfs);

    return 0;
}

core_initcall(ramfs_register);
