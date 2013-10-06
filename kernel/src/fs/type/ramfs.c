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
    if(!strcmp(dentry->name, "dev")) {
        dentry->fs = inode->fs;
        dentry->inode = inode_alloc(inode->fs, inode->ops);
        dentry->inode->mode = 0755;
    }
}

static dentry_t * ramfs_inode_mkdir(inode_t *inode, char *name) {
    return NULL;
}

static inode_ops_t ramfs_inode_ops = {
    .file_ops = &ramfs_file_ops,

    .lookup = ramfs_inode_lookup,
    .mkdir  = ramfs_inode_mkdir,
};

static dentry_t * ramfs_mount(fs_type_t *type, const char *device);

static fs_type_t ramfs = {
    .name  = "ramfs",
    .flags = FSTYPE_FLAG_NODEV,
    .mount  = ramfs_mount,
};

static void ramfs_fill(fs_t *fs) {
    dentry_t *root = fs->root = dentry_alloc("");
    root->fs = fs;

    root->inode = inode_alloc(fs, &ramfs_inode_ops);
    root->inode->mode = 0755;
}

static dentry_t * ramfs_mount(fs_type_t *type, const char *device) {
    return mount_nodev(type, ramfs_fill);
}

static INITCALL ramfs_register() {
    register_fs_type(&ramfs);

    return 0;
}

core_initcall(ramfs_register);
