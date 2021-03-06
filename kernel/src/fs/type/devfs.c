#include "init/initcall.h"
#include "init/param.h"
#include "common/list.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "sched/sched.h"
#include "sched/ktaskd.h"
#include "fs/vfs.h"
#include "fs/type/devfs.h"
#include "log/log.h"

static fs_t *devfs;
static thread_t *devfs_task;
static DEFINE_LIST(devfs_devices);
static DEFINE_LIST(devfs_pending);
static DEFINE_SPINLOCK(devfs_lock);
static const char *mntpoint = "/dev";

static void char_file_open(file_t *file, inode_t *inode) {
    devfs_device_t *device = inode->private;
    file->private = device;
}

static off_t char_file_seek(file_t *file, off_t off, int whence) {
    return 0;
}

static ssize_t char_file_read(file_t *file, char *buff, size_t bytes) {
    devfs_device_t *device = file->private;
    char_device_t *cdev = device->chardev;

    return cdev->ops->read(cdev, buff, bytes);
}

static ssize_t char_file_write(file_t *file, const char *buff, size_t bytes) {
    devfs_device_t *device = file->private;
    char_device_t *cdev = device->chardev;

    return cdev->ops->write(cdev, buff, bytes);
}

static int32_t char_file_poll(file_t *file, fpoll_data_t *fd) {
    devfs_device_t *device = file->private;
    char_device_t *cdev = device->chardev;

    return cdev->ops->poll(cdev, fd);
}

static void char_file_close(file_t *file) {
}

static file_ops_t char_file_ops = {
    .open = char_file_open,
    .close = char_file_close,
    .seek = char_file_seek,
    .read = char_file_read,
    .write = char_file_write,
    .poll = char_file_poll,
};

static inode_ops_t char_inode_ops = {
    .file_ops = &char_file_ops,
};

static void block_file_open(file_t *file, inode_t *inode) {
    devfs_device_t *device = inode->private;
    file->private = device;
}

static off_t block_file_seek(file_t *file, off_t off, int whence) {
    return 0;
}

static ssize_t block_file_read(file_t *file, char *buff, size_t bytes) {
    //TODO implement buffered IO scheduler for block abstracted reads
    return -1;
}

static ssize_t block_file_write(file_t *file, const char *buff, size_t bytes) {
    //TODO implement buffered IO scheduler for block abstracted writes
    return -1;
}

static int32_t block_file_poll(file_t *file, fpoll_data_t *fd) {
    //TODO implement me
    return -1;
}

static void block_file_close(file_t *file) {
}

static file_ops_t block_file_ops = {
    .open = block_file_open,
    .close = block_file_close,
    .seek = block_file_seek,
    .read = block_file_read,
    .write = block_file_write,
    .poll = block_file_poll,
};

static inode_ops_t block_inode_ops = {
    .file_ops = &block_file_ops,
};

static void devfs_root_file_open(file_t *file, inode_t *inode) {
}

static void devfs_root_file_close(file_t *file) {
}

static file_ops_t devfs_root_file_ops = {
    .open   = devfs_root_file_open,
    .close  = devfs_root_file_close,

    .iterate = simple_file_iterate,
};

static void devfs_root_inode_lookup(inode_t *inode, dentry_t *target) {
}

static inode_ops_t devfs_root_inode_ops = {
    .file_ops = &devfs_root_file_ops,

    .lookup = devfs_root_inode_lookup,
    .create = fs_no_create,

    //FIXME define devfs_root_inode_getattr
    //.getattr = devfs_root_inode_getattr,
};

static inode_t * devfs_inode_alloc(devfs_device_t *device) {
    inode_t *inode;

    switch(device->type) {
        case CHAR_DEV: {
            inode = inode_alloc(devfs, &char_inode_ops);
            inode->mode = S_IFCHR | 0755;
            break;
        }
        case BLCK_DEV: {
            inode = inode_alloc(devfs, &block_inode_ops);
            inode->mode = S_IFBLK | 0755;
            break;
        }
        default: {
            panicf("devfs - unknown device type %X", device->type);
        }
    }

    //FIXME sensibly populate these
    inode->uid = 0;
    inode->gid = 0;
    inode->nlink = 0;
    inode->dev = 0;
    inode->rdev = 0;

    inode->atime = 0;
    inode->mtime = 0;
    inode->ctime = 0;

    inode->blkshift = 12;
    inode->blocks = 8;

    inode->size = 0;

    inode->private = device;

    return inode;
}

static void devfs_fill(fs_t *fs) {
    fs->root = dentry_alloc("");
    fs->root->fs = fs;

    inode_t *inode = inode_alloc(fs, &devfs_root_inode_ops);
    fs->root->inode = inode;
    fs->root->inode->flags |= INODE_FLAG_DIRECTORY;

    inode->mode = S_IFDIR | 0755;

    //FIXME sensibly populate these
    inode->uid = 0;
    inode->gid = 0;
    inode->nlink = 0;
    inode->dev = 0;
    inode->rdev = 0;

    inode->atime = 0;
    inode->mtime = 0;
    inode->ctime = 0;

    inode->blkshift = 12;
    inode->blocks = 8;

    inode->size = 1 << inode->blkshift;
}

static dentry_t * devfs_create(fs_type_t *fs_type, const char *device) {
    return fs_create_single(fs_type, devfs_fill);
}

static fs_type_t devfs_type = {
    .name  = "devfs",
    .flags = FSTYPE_FLAG_NODEV,
    .create  = devfs_create,
};

static void devfs_add_dev(void *device, device_type_t type, char *name) {
    devfs_device_t *d = kmalloc(sizeof(devfs_device_t));
    d->name = name;
    d->dentry = dentry_alloc(d->name);

    d->type = type;
    d->dev = device;

    uint32_t flags;
    spin_lock_irqsave(&devfs_lock, &flags);
    list_add_before(&d->list, &devfs_pending);

    if(devfs_task) thread_wake(devfs_task);

    spin_unlock_irqstore(&devfs_lock, flags);
}

static char * get_strpath(const char *name) {
    uint32_t mntlen = strlen(mntpoint);
    uint32_t namelen = strlen(name);

    char *str = kmalloc(mntlen + namelen + 2);
    memcpy(str, mntpoint, mntlen);
    memcpy(str + mntlen, "/", 1);
    memcpy(str + mntlen + 1, name, namelen);
    str[mntlen + namelen + 1] = '\0';

    return str;
}

int32_t devfs_lookup(const char *name, path_t *out) {
    char *path = get_strpath(name);
    int32_t ret = vfs_lookup(NULL, path, out);
    kfree(path);
    return ret;
}

void devfs_add_chardev(char_device_t *device, char *name) {
    devfs_add_dev(device, CHAR_DEV, name);
}

void devfs_add_blockdev(block_device_t *device, char *name) {
    devfs_add_dev(device, BLCK_DEV, name);
}

static bool devfs_set_mntpoint(char *point) {
    mntpoint = point;

    return true;
}

cmdline_param("devfs.mount", devfs_set_mntpoint);

static int32_t create_path(path_t *start, const char *orig_path) {
    char *path = strdup(orig_path);
    char *part = path;

    while(part && *part) {
        part = strchr(part, '/');

        if(part) part[0] = '\0';
        int32_t ret = vfs_create(start, path, S_IFDIR | 0755, NULL);
        if(ret) {
            return ret;
        }
        if(part) {
            part[0] = '/';
            part++;
        }
    }

    kfree(path);

    return 0;
}

//called with devfs_lock held
static bool devfs_update() {
    if(list_empty(&devfs_pending)) {
        return false;
    }

    devfs_device_t *d = list_first(&devfs_pending, devfs_device_t, list);
    list_rm(&d->list);

    d->dentry->inode = devfs_inode_alloc(d);
    dentry_activate(d->dentry, devfs->root);

    list_add(&d->list, &devfs_devices);
    kprintf("devfs - added \"%s/%s\"", mntpoint, d->dentry->name);

    return true;
}

//called with devfs_lock held
void devfs_publish_pending() {
    while(devfs_update());
}

static void devfs_run(void *UNUSED(arg)) {
    irqenable();

    while(true) {
        spin_lock_irq(&devfs_lock);

        devfs_publish_pending();
        thread_sleep_prepare();

        spin_unlock_irq(&devfs_lock);

        sched_switch();
    }
}

static INITCALL devfs_init() {
    register_fs_type(&devfs_type);

    return 0;
}

static INITCALL devfs_mount() {
    devfs = vfs_fs_create("devfs", NULL);

    devfs_publish_pending();

    if(mntpoint) {
        path_t wd, target;
        wd.mount = root_mount;
        wd.dentry = root_mount->fs->root;

        int32_t ret;
        if((ret = create_path(&wd, mntpoint))) {
            kprintf("devfs - could not create path \"%s\"", mntpoint);
        } else if((ret = vfs_lookup(&wd, mntpoint, &target))) {
            kprintf("devfs - could not lookup \"%s\": %d", mntpoint, ret);
        } else if(vfs_do_mount(devfs, &target)) {
            kprintf("devfs - mounted at \"%s\"", mntpoint);
        } else {
            kprintf("devfs - could not mount at \"%s\"", mntpoint);
        }
    }

    ktaskd_request("kdevfsd", devfs_run, NULL);

    return 0;
}

core_initcall(devfs_init);
fs_initcall(devfs_mount);
