#include "lib/int.h"
#include "common/init.h"
#include "common/list.h"
#include "common/hash.h"
#include "common/hashtable.h"
#include "common/listener.h"
#include "sync/spinlock.h"
#include "bug/panic.h"
#include "fs/vfs.h"
#include "video/log.h"

static dentry_t root = {
    .name = "",

    .parent = NULL,
    .children = LIST_HEAD(root.children),
    .siblings = LIST_HEAD(root.siblings),
};

static DEFINE_LIST(disk_labels);
static DEFINE_SPINLOCK(disk_label_lock);

static DEFINE_HASHTABLE(fs_types, 5);
static DEFINE_SPINLOCK(fs_type_lock);

void register_block_device(block_device_t *device, char *name) {
    //TODO add this device to /dev
}

void register_disk(block_device_t *device) {
    //FIXME after implementing an IO scheduler
    //uint32_t flags;
    //spin_lock_irqsave(&disk_label_lock, &flags);

    disk_label_t *disk_label;
    LIST_FOR_EACH_ENTRY(disk_label, &disk_labels, list) {
        if(disk_label->probe(device)) {
            break;
        }
    }

    //spin_unlock_irqstore(&disk_label_lock, flags);
}

void register_disk_label(disk_label_t *disk_label) {
    uint32_t flags;
    spin_lock_irqsave(&disk_label_lock, &flags);

    list_add(&disk_label->list, &disk_labels);

    spin_unlock_irqstore(&disk_label_lock, flags);
}

void register_fs_type(fs_type_t *fs_type) {
    uint32_t flags;
    spin_lock_irqsave(&fs_type_lock, &flags);

    hashtable_add(str_to_key(fs_type->name), &fs_type->node, fs_types);

    spin_unlock_irqstore(&fs_type_lock, flags);
}

fs_type_t * find_fs_type(const char *name) {
    uint32_t flags;
    spin_lock_irqsave(&fs_type_lock, &flags);

    fs_type_t *type;
    HASHTABLE_FOR_EACH_COLLISION(str_to_key(name), type, fs_types, node) {
        if(!strcmp(name, type->name)) {
            return type;
        }
    }

    spin_unlock_irqstore(&fs_type_lock, flags);

    return NULL;
}

static void swap_dentries(dentry_t *old, dentry_t *new) {
    if(!list_empty(&old->children)) {
        list_replace(&old->children, &new->children);
    }

    if(!list_empty(&old->siblings)) {
        list_replace(&old->siblings, &new->siblings);
    }
}

bool mount(block_device_t *device, const char *raw_type, dentry_t *mountpoint) {
    fs_type_t *type = find_fs_type(raw_type);
    if(!type || (device && type->flags & FSTYPE_FLAG_NODEV) || (!device && !(type->flags & FSTYPE_FLAG_NODEV))) {
        return false;
    }

    fs_t *fs = type->open(device);
    if(!fs) return false;

    fs->mountpoint = mountpoint;
    fs->root->parent = mountpoint->parent;

    //FIXME the below should be atomic

    if(device) device->fs = fs;
    swap_dentries(mountpoint, fs->root);

    return true;
}

bool umount(dentry_t *d) {
    if(!d->parent) panic("cannot unmount /");

    fs_t *fs = d->inode->fs;
    if(d->inode == fs->root->inode) {
        swap_dentries(fs->root, fs->mountpoint);
    } else {
        //TODO check if this is the dentry for the block_device_t
        return false;
    }

    return true;
}

static INITCALL vfs_init() {
    return mount(NULL, "ramfs", &root) ? 0 : 1;
}

device_initcall(vfs_init);
