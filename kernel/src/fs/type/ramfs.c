#include "lib/int.h"
#include "lib/string.h"
#include "common/init.h"
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
    dentry->inode = kmalloc(sizeof(inode_t));
    dentry->inode->fs = inode->fs;
    dentry->inode->ops = inode->ops;
}

static dentry_t * ramfs_inode_mkdir(inode_t *inode, char *name) {
    return NULL;
}

static inode_ops_t ramfs_inode_ops = {
    .file_ops = &ramfs_file_ops,

    .lookup = ramfs_inode_lookup,
    .mkdir  = ramfs_inode_mkdir,
};

static fs_t * ramfs_open(block_device_t *device) {
    fs_t *fs = kmalloc(sizeof(fs_t));

    dentry_t *root = kmalloc(sizeof(dentry_t));
    root->name = "";
    hashtable_init(root->children);
    list_init(&root->siblings);

    dentry_t *dev = kmalloc(sizeof(dentry_t));
    dev->name = "dev";
    hashtable_init(dev->children);
    list_init(&dev->siblings);

    hashtable_add(str_to_key(dev->name, strlen(dev->name)), &dev->node, root->children);

    fs->root = root;

    inode_t *root_inode = kmalloc(sizeof(inode_t));
    root->inode = root_inode;

    root_inode->fs = fs;
    root_inode->ops = &ramfs_inode_ops;

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
