#include "init/initcall.h"
#include "init/param.h"
#include "common/list.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "sched/sched.h"
#include "sched/task.h"
#include "fs/vfs.h"
#include "fs/type/devfs.h"
#include "video/log.h"

typedef struct devfs_device {
    list_head_t list;

    char *name;
    block_device_t *device;
} devfs_device_t;

static fs_t *devfs;
static task_t *devfs_task;
static DEFINE_LIST(devfs_devices);
static DEFINE_LIST(devfs_pending);
static DEFINE_SPINLOCK(devfs_lock);

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
    inode_t *inode = inode_alloc(devfs, &devfs_dev_inode_ops);
    inode->private = device;

    return inode;
}

static dentry_t * devfs_create(fs_type_t *fs_type, const char *device);

static fs_type_t devfs_type = {
    .name  = "devfs",
    .flags = FSTYPE_FLAG_NODEV,
    .create  = devfs_create,
};

static void devfs_fill(fs_t *fs) {
    fs->root = dentry_alloc("");
    fs->root->fs = fs;
    fs->root->inode = inode_alloc(fs, &root_inode_ops);
    fs->root->inode->flags |= INODE_FLAG_DIRECTORY;
}

static dentry_t * devfs_create(fs_type_t *fs_type, const char *device) {
    return fs_create_single(fs_type, devfs_fill);
}

void devfs_add_device(block_device_t *device, char *name) {
    devfs_device_t *d = kmalloc(sizeof(devfs_device_t));
    d->name = name;
    d->device = device;
    device->dentry = dentry_alloc(d->name);

    uint32_t flags;
    spin_lock_irqsave(&devfs_lock, &flags);
    list_add_before(&d->list, &devfs_pending);
    spin_unlock_irqstore(&devfs_lock, flags);

    if(devfs_task) task_wake(devfs_task);
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

    while(part && *part) {
        part = strchr(part, '/');

        if(part) part[0] = '\0';
        if(!vfs_mkdir(start, path, 0755)) return false;
        if(part) {
            part[0] = '/';
            part++;
        }
    }

    kfree(path, strlen(orig_path) + 1);

    return true;
}

static bool devfs_update() {
    uint32_t flags;
    spin_lock_irqsave(&devfs_lock, &flags);

    if(list_empty(&devfs_pending)) {
        spin_unlock_irqstore(&devfs_lock, flags);

        return false;
    }

    devfs_device_t *d = list_first(&devfs_pending, devfs_device_t, list);
    list_rm(&d->list);

    spin_unlock_irqstore(&devfs_lock, flags);

    d->device->dentry->inode = devfs_inode_alloc(d);
    dentry_add_child(d->device->dentry, devfs->root);

    spin_lock_irqsave(&devfs_lock, &flags);

    list_add(&d->list, &devfs_devices);
    kprintf("devfs - added /dev/%s", d->device->dentry->name);

    spin_unlock_irqstore(&devfs_lock, flags);

    return true;
}

static void devfs_run() {
    while(true) {
        while(devfs_update());
        task_sleep_current();
    }
}

static INITCALL devfs_init() {
    register_fs_type(&devfs_type);

    return 0;
}

static INITCALL devfs_mount() {
    devfs = vfs_fs_create("devfs", NULL);
    devfs_task = task_create("devfsd", true, devfs_run, NULL);

    while(devfs_update());

    if(mntpoint) {
        path_t wd, target;
        wd.mount = root_mount;
        wd.dentry = root_mount->fs->root;

        if(!create_path(&wd, mntpoint)) {
            kprintf("devfs - could not create path \"%s\"", mntpoint);
        } else if(!vfs_lookup(&wd, mntpoint, &target)) {
            kprintf("devfs - could not lookup \"%s\"", mntpoint);
        } else if(vfs_do_mount(devfs, &target)) {
            kprintf("devfs - mounted at \"%s\"", mntpoint);
        } else {
            kprintf("devfs - could not mount at \"%s\"", mntpoint);
        }
    }

    task_add(devfs_task);

    return 0;
}

core_initcall(devfs_init);
fs_initcall(devfs_mount);
