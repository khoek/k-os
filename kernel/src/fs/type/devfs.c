#include "init/initcall.h"
#include "init/param.h"
#include "common/list.h"
#include "mm/cache.h"
#include "fs/vfs.h"
#include "fs/type/devfs.h"
#include "video/log.h"

typedef struct devfs_device {
    list_head_t list;

    char *name;
    block_device_t *device;
} devfs_device_t;

static dentry_t *root;
static DEFINE_LIST(devfs_devices);

static void devfs_dev_file_open(file_t *file, inode_t *inode) {
}

static void devfs_dev_file_close(file_t *file) {
}

static file_ops_t devfs_dev_file_ops = {
    .open   = devfs_dev_file_open,
    .close  = devfs_dev_file_close,
};

static inode_ops_t devfs_dev_inode_ops = {
    .file_ops = &devfs_dev_file_ops,
};

static void root_inode_lookup(inode_t *inode, dentry_t *target) {
}

static inode_ops_t root_inode_ops = {
    .lookup = root_inode_lookup,
};

static inode_t * devfs_inode_alloc(devfs_device_t *device) {
    inode_t *inode = inode_alloc(root->fs, &devfs_dev_inode_ops);
    inode->private = device;

    return inode;
}

static dentry_t * devfs_mount(fs_type_t *fs_type, const char *device);

static fs_type_t devfs = {
    .name  = "devfs",
    .flags = FSTYPE_FLAG_NODEV,
    .mount  = devfs_mount,
};

static void devfs_fill(fs_t *fs) {
    fs->root = root = dentry_alloc("");
    root->inode = inode_alloc(fs, &root_inode_ops);

    devfs_device_t *device;
    LIST_FOR_EACH_ENTRY(device, &devfs_devices, list) {
        dentry_t *new = device->device->dentry;
        new->inode = devfs_inode_alloc(device);

        dentry_add_child(new, root);
    }
}

static dentry_t * devfs_mount(fs_type_t *fs_type, const char *device) {
    return mount_single(fs_type, devfs_fill);
}

void devfs_add_device(block_device_t *device, char *name) {
    devfs_device_t *d = kmalloc(sizeof(devfs_device_t));
    d->name = name;
    d->device = device;
    list_add(&d->list, &devfs_devices);

    device->dentry = dentry_alloc(d->name);

    if(root) {
        device->dentry->inode = devfs_inode_alloc(d);
        dentry_add_child(device->dentry, root);
    }

    logf("devfs - added /dev/%s", name);
}

static char *mntpoint;

static bool devfs_set_mntpoint(char *point) {
    mntpoint = point;

    return true;
}

cmdline_param("devfs.mount", devfs_set_mntpoint);

static bool create_path(path_t *start, char *orig_path) {
    char *path = strdup(orig_path);
    char *part = path;

    while(*part) {
        part = strchr(part, '/');

        if(part) part[0] = '\0';
        if(!vfs_mkdir(start, path, 0755)) return false;
        if(part) part[0] = '/';

        part++;
    }

    kfree(path, strlen(orig_path) + 1);

    return true;
}

static INITCALL devfs_init() {
    register_fs_type(&devfs);

    return 0;
}

static INITCALL devfs_automount() {
    if(mntpoint) {
        path_t wd, target;
        wd.mount = root_mount;
        wd.dentry = root_mount->fs->root;

        create_path(&wd, mntpoint);

        if(!vfs_lookup(&wd, mntpoint, &target)) {
            logf("devfs - could not lookup \"%s\"", mntpoint);
        } else if(!vfs_mount("devfs", NULL, &target)) {
            logf("devfs - could not mount at \"%s\"", mntpoint);
        } else {
            logf("devfs - mounted at \"%s\"", mntpoint);
        }
    }

    return 0;
}

core_initcall(devfs_init);
fs_initcall(devfs_automount);
