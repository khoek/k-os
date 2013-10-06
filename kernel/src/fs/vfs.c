#include "lib/int.h"
#include "init/initcall.h"
#include "common/math.h"
#include "common/list.h"
#include "common/hash.h"
#include "common/hashtable.h"
#include "common/listener.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "fs/fd.h"
#include "fs/vfs.h"
#include "video/log.h"

static cache_t *dentry_cache;
static cache_t *inode_cache;
static cache_t *file_cache;
static cache_t *fs_cache;
static cache_t *mount_cache;

static dentry_t *root;

static DEFINE_HASHTABLE(fs_types, 5);
static DEFINE_SPINLOCK(fs_type_lock);

static DEFINE_HASHTABLE(mount_hashtable, log2(PAGE_SIZE / sizeof(hashtable_node_t)));
static DEFINE_SPINLOCK(mount_hashtable_lock);

dentry_t * dentry_alloc(const char *name) {
    dentry_t *new = cache_alloc(dentry_cache);
    new->name = name;
    new->child = NULL;
    new->inode = NULL;
    new->flags = 0;
    hashtable_init(new->children);
    list_init(&new->siblings);

    return new;
}

inode_t * inode_alloc(fs_t *fs, inode_ops_t *ops) {
    inode_t *new = cache_alloc(inode_cache);
    memset(new, 0, sizeof(sizeof(inode_t)));

    new->fs = fs;
    new->ops = ops;

    return new;
}

file_t * file_alloc(file_ops_t *ops) {
    file_t *new = cache_alloc(file_cache);
    new->ops = ops;

    return new;
}

static fs_t * fs_alloc(fs_type_t *type) {
    fs_t *new = cache_alloc(fs_cache);
    new->type = type;

    return new;
}

static mount_t * mount_alloc(mount_t *parent, fs_t *fs, dentry_t *mountpoint) {
    mount_t *mount = cache_alloc(mount_cache);
    mount->parent = parent;
    mount->fs = fs;
    mount->mountpoint = mountpoint;

    return mount;
}

void dentry_add_child(dentry_t *child, dentry_t *parent) {
    child->parent = parent;

    hashtable_add(str_to_key(child->name, strlen(child->name)), &child->node, parent->children);

    if(parent->child) {
        list_add(&child->siblings, &parent->child->siblings);
    } else {
        list_init(&child->siblings);
        parent->child = child;
    }
}

void register_fs_type(fs_type_t *fs_type) {
    list_init(&fs_type->instances);

    uint32_t flags;
    spin_lock_irqsave(&fs_type_lock, &flags);

    hashtable_add(str_to_key(fs_type->name, strlen(fs_type->name)), &fs_type->node, fs_types);

    spin_unlock_irqstore(&fs_type_lock, flags);
}

fs_type_t * find_fs_type(const char *name) {
    uint32_t flags;
    spin_lock_irqsave(&fs_type_lock, &flags);

    fs_type_t *type;
    HASHTABLE_FOR_EACH_COLLISION(str_to_key(name, strlen(name)), type, fs_types, node) {
        if(!strcmp(name, type->name)) {
            return type;
        }
    }

    spin_unlock_irqstore(&fs_type_lock, flags);

    return NULL;
}

static uint32_t hash_mount(mount_t *mount, dentry_t *mountpoint) {
    return ((uint32_t) mount) * ((uint32_t) mountpoint);
}

static mount_t * get_mount(path_t *mountpoint) {
    uint32_t flags;
    spin_lock_irqsave(&mount_hashtable_lock, &flags);

    mount_t *mount;
    HASHTABLE_FOR_EACH_COLLISION(hash_mount(mountpoint->mount, mountpoint->dentry), mount, mount_hashtable, node) {
        if(mount->parent == mountpoint->mount && mount->mountpoint == mountpoint->dentry) {
            goto get_mount_out;
        }
    }

    mount = NULL;

get_mount_out:
    spin_unlock_irqstore(&mount_hashtable_lock, flags);

    return mount;
}

bool vfs_mount(const char *raw_type, const char *device, path_t *mountpoint) {
    if(!(mountpoint->dentry->flags & DENTRY_FLAG_DIRECTORY)) return false;
    if(get_mount(mountpoint)) return false;

    fs_type_t *type = find_fs_type(raw_type);
    if(!type) return false;

    dentry_t *root = type->mount(type, device);
    if(!root) return false;

    fs_t *fs = root->fs;
    BUG_ON(!fs);

    mount_t *mount = mount_alloc(mountpoint->mount, fs, mountpoint->dentry);

    uint32_t flags;
    spin_lock_irqsave(&mount_hashtable_lock, &flags);

    hashtable_add(hash_mount(mount->parent, mountpoint->dentry), &mount->node, mount_hashtable);

    spin_unlock_irqstore(&mount_hashtable_lock, flags);

    mountpoint->dentry->flags |= DENTRY_FLAG_MOUNTPOINT;

    return true;
}

bool vfs_umount(path_t *mountpoint) {
    if(mountpoint->dentry->flags & DENTRY_FLAG_MOUNTPOINT) {
        mountpoint->dentry->flags &= ~DENTRY_FLAG_MOUNTPOINT;

        mount_t *mount = get_mount(mountpoint);

        BUG_ON(!mount);

        uint32_t flags;
        spin_lock_irqsave(&mount_hashtable_lock, &flags);

        hashtable_rm(&mount->node);

        spin_unlock_irqstore(&mount_hashtable_lock, flags);

        return true;
    } else {
        //TODO check if this is the dentry for a device somehow
    }

    return false;
}

bool vfs_lookup(path_t *start, const char *path, path_t *out) {
    mount_t *mount = start->mount;
    dentry_t *wd = start->dentry;

    if(*path == PATH_SEPARATOR) {
        wd = root;
        path++;
    }

    bool finished = false;
    while(!finished && *path) {
        while(true) {
            if(!wd) return false;

            if(!(wd->flags & DENTRY_FLAG_MOUNTPOINT)) {
                break;
            }

            path_t path;
            path.mount = mount;
            path.dentry = wd;

            mount_t *submount = get_mount(&path);
            BUG_ON(!submount);

            mount = submount;
            wd = submount->fs->root;
        }

        BUG_ON(!wd->inode);

        uint32_t len = 0;
        const char *next = path;
        while(*next != PATH_SEPARATOR) {
            next++;
            len++;

            if(!*next) {
                finished = true;
                break;
            }
        }

        if(!finished && !(wd->flags & DENTRY_FLAG_DIRECTORY)) return false;

        next++;

        dentry_t *next_dentry = wd;

        if(!len) goto lookup_next;

        if(next[0] == '.') switch(len) {
            case 1: {
                goto lookup_next;
            }
            case 2: {
                if(next[1] == '.') {
                    if(wd->parent == NULL) {
                        if(wd->flags & DENTRY_FLAG_MOUNTPOINT) {
                            BUG_ON(!mount->parent);
                            wd = mount->mountpoint;
                            mount = mount->parent;
                        } else return false;
                    } else {
                        next_dentry = wd->parent;
                    }

                    goto lookup_next;
                }
            }
        }

        HASHTABLE_FOR_EACH_COLLISION(str_to_key(path, len), next_dentry, wd->children, node) {
            if(!memcmp(next_dentry->name, path, len)) {
                goto lookup_next;
            }
        }

        next_dentry = dentry_alloc(path);
        wd->inode->ops->lookup(wd->inode, next_dentry);

        if(next_dentry->inode) goto lookup_next;

        dentry_add_child(next_dentry, wd);

        return false;

lookup_next:
        wd = next_dentry;
        path = next;
    }

    out->mount = mount;
    out->dentry = wd;

    return true;
}

gfd_idx_t vfs_open_file(inode_t *inode) {
    file_t *file = file_alloc(inode->ops->file_ops);
    gfd_idx_t gfd = gfdt_add(file);

    if(gfd != FD_INVALID) {
        file->ops->open(file, inode);
    }

    return gfd;
}

static void fs_add(fs_t *fs) {
    list_add(&fs->list, &fs->type->instances);
}

static block_device_t * path_to_dev(path_t *path) {
    return path->dentry->inode->device.block;
}

dentry_t * mount_dev(fs_type_t *type, const char *device, void (*fill)(fs_t *fs, block_device_t *device)) {
    path_t path;
    if(!vfs_lookup(&current->pwd, device, &path)) return NULL;
    block_device_t *blk_device = path_to_dev(&path);
    if(!blk_device) return NULL;

    fs_t *fs = fs_alloc(type);
    fill(fs, blk_device);

    fs_add(fs);

    BUG_ON(!fs->root);

    return fs->root;
}

dentry_t * mount_nodev(fs_type_t *type, void (*fill)(fs_t *fs)) {
    fs_t *fs = fs_alloc(type);
    fill(fs);

    fs_add(fs);

    return fs->root;
}

dentry_t * mount_single(fs_type_t *type, void (*fill)(fs_t *fs)) {
    if(!list_empty(&type->instances)) {
        return list_first(&type->instances, fs_t, list)->root;
    }

    fs_t *fs = fs_alloc(type);
    fill(fs);

    fs_add(fs);

    return fs->root;
}

void vfs_getattr(dentry_t *dentry, stat_t *stat) {
    if(dentry->inode->ops->getattr) dentry->inode->ops->getattr(dentry, stat);
    else generic_getattr(dentry->inode, stat);
}

void generic_getattr(inode_t *inode, stat_t *stat) {
    stat->st_dev = inode->dev;
    stat->st_ino = inode->ino;
    stat->st_mode = inode->mode;
    stat->st_nlink = inode->nlink;
    stat->st_uid = inode->uid;
    stat->st_gid = inode->gid;
    stat->st_rdev = inode->rdev;
    stat->st_size = inode->size;

    stat->st_atime = inode->atime;
    stat->st_mtime = inode->mtime;
    stat->st_ctime = inode->ctime;

    stat->st_blksize = 1 << inode->blkshift;
    stat->st_blocks = inode->blocks;
}

static INITCALL vfs_init() {
    dentry_cache = cache_create(sizeof(dentry_t));
    inode_cache = cache_create(sizeof(inode_t));
    file_cache = cache_create(sizeof(file_t));
    fs_cache = cache_create(sizeof(fs_t));
    mount_cache = cache_create(sizeof(mount_t));

    return 0;
}

static INITCALL vfs_root_mount() {
    root = dentry_alloc("");
    root->parent = NULL;
    root->flags |= DENTRY_FLAG_DIRECTORY;

    path_t root_path;
    root_path.mount = NULL;
    root_path.dentry = root;

    logf("mounting root fs");

    return vfs_mount("ramfs", NULL, &root_path) ? 0 : 1;
}

core_initcall(vfs_init);
device_initcall(vfs_root_mount);
