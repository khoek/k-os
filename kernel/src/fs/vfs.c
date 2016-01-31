#include "common/types.h"
#include "lib/string.h"
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
#include "sched/sched.h"
#include "fs/fd.h"
#include "fs/vfs.h"
#include "log/log.h"

mount_t *root_mount;

static cache_t *dentry_cache;
static cache_t *inode_cache;
static cache_t *fs_cache;
static cache_t *mount_cache;

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

void dentry_free(dentry_t *dentry) {
    kfree((char *) dentry->name);

    cache_free(dentry_cache, dentry);
}

inode_t * inode_alloc(fs_t *fs, inode_ops_t *ops) {
    inode_t *new = cache_alloc(inode_cache);
    memset(new, 0, sizeof(inode_t));

    new->fs = fs;
    new->ops = ops;

    return new;
}

file_t * file_alloc(file_ops_t *ops) {
    file_t *new = gfdt_obtain();
    if(new) {
        new->offset = 0;
        new->ops = ops;
    }

    return new;
}

static fs_t * fs_alloc(fs_type_t *type) {
    fs_t *new = cache_alloc(fs_cache);
    new->type = type;

    return new;
}

static mount_t * mount_alloc(fs_t *fs) {
    mount_t *mount = cache_alloc(mount_cache);
    mount->mountpoint = NULL;
    mount->parent = NULL;
    mount->fs = fs;

    return mount;
}

void dentry_activate(dentry_t *child, dentry_t *parent) {
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
            goto fs_found;
        }
    }

    type = NULL;

fs_found:
    spin_unlock_irqstore(&fs_type_lock, flags);

    return type;
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

mount_t * vfs_mount(const char *raw_type, const char *device, path_t *mountpoint) {
    fs_t *fs = vfs_fs_create(raw_type, device);
    if(!fs) return false;

    return vfs_do_mount(fs, mountpoint);
}

fs_t * vfs_fs_create(const char *raw_type, const char *device) {
    fs_type_t *type = find_fs_type(raw_type);
    if(!type) return false;

    dentry_t *root = type->create(type, device);
    if(!root) return false;

    fs_t *fs = root->fs;
    BUG_ON(!fs);

    return fs;
}

void vfs_fs_destroy(fs_t *fs) {
    //TODO implement this
}

mount_t * vfs_do_mount(fs_t *fs, path_t *mountpoint) {
    mount_t *mount = vfs_mount_create(fs);
    if(mount) {
        if(!vfs_mount_add(mount, mountpoint)) {
            vfs_mount_destroy(mount);
            mount = NULL;
        }

        goto mount_success;
    }

    vfs_fs_destroy(fs);

mount_success:
    return mount;
}

mount_t * vfs_mount_create(fs_t *fs) {
    return mount_alloc(fs);
}

bool vfs_mount_add(mount_t *mount, path_t *mountpoint) {
    if(!(mountpoint->dentry->inode->flags & INODE_FLAG_DIRECTORY)) return false;
    if(get_mount(mountpoint)) return false;

    mount->parent = mountpoint->mount;
    mount->mountpoint = mountpoint->dentry;

    uint32_t flags;
    spin_lock_irqsave(&mount_hashtable_lock, &flags);

    hashtable_add(hash_mount(mount->parent, mount->mountpoint), &mount->node, mount_hashtable);

    spin_unlock_irqstore(&mount_hashtable_lock, flags);

    mount->mountpoint->inode->flags |= INODE_FLAG_MOUNTPOINT;

    return true;
}

void vfs_mount_destroy(mount_t *mount) {
    //TODO implement this
}

bool vfs_umount(path_t *mountpoint) {
    if(mountpoint->dentry->inode->flags & INODE_FLAG_MOUNTPOINT) {
        mountpoint->dentry->inode->flags &= ~INODE_FLAG_MOUNTPOINT;

        mount_t *mount = get_mount(mountpoint);

        BUG_ON(!mount);
        BUG_ON(mount == root_mount);

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

static char * last_segment(const char *path) {
    const char *seg = path;
    while(*path) {
        if(*path == PATH_SEPARATOR && *(path + 1)) seg = path + 1;
        path++;
    }

    return (char *) seg;
}

static bool get_path_wd(const path_t *start, const char *orig_pathname, path_t *wd, char **out_last) {
    char *pathname = strdup(orig_pathname);
    char *last = last_segment(pathname);

    if(strcmp(pathname, last)) {
        *(last - 1) = '\0';

        path_t path;
        if(!vfs_lookup(start, pathname, &path)) {
            kfree(pathname);
            return false;
        }

        *wd = path;
    } else {
        *wd = *start;
    }

    *out_last = strdup(last);

    kfree(pathname);

    return true;
}


bool vfs_create(const path_t *start, const char *pathname, uint32_t mode, bool excl) {
    path_t wd;
    char *last;
    if(!get_path_wd(start, pathname, &wd, &last)) {
        return false;
    }

    path_t f;
    if(vfs_lookup(&wd, last, &f)) {
        if(excl) {
            kfree(last);
            return false;
        }

        //otherwise, do nothing
    } else {
        char *copy = strdup(last);
        dentry_t *new = dentry_alloc(copy);
        wd.dentry->inode->ops->create(wd.dentry->inode, new, mode);
        if(!new->inode)  {
            kfree(copy);
            return false;
        }

        dentry_activate(new, wd.dentry);
    }

    return true;

}

bool vfs_mkdir(const path_t *start, const char *pathname, uint32_t mode) {
    path_t wd;
    char *last = NULL;
    if(!get_path_wd(start, pathname, &wd, &last)) {
        kfree(last);
        return false;
    }

    char *copy = strdup(last);
    dentry_t *newdir = dentry_alloc(copy);
    wd.dentry->inode->ops->mkdir(wd.dentry->inode, newdir, mode);
    if(!newdir->inode)  {
        kfree(copy);
        return false;
    }

    dentry_activate(newdir, wd.dentry);

    return true;
}

bool vfs_lookup(const path_t *start, const char *orig_path, path_t *out) {
    char *path = strdup(orig_path);
    char *new_path = path;

    mount_t *mount;
    dentry_t *wd;

    if(*path != PATH_SEPARATOR && start) {
        mount = start->mount;
        wd = start->dentry;
    } else {
        if(*path == PATH_SEPARATOR) {
            path++;
        }

        mount = root_mount;
        wd = root_mount->fs->root;
    }

    bool finished = false;
    while(!finished && *path && wd) {
        char *next = path;

        BUG_ON(!wd->inode);

        if(!(wd->inode->flags & INODE_FLAG_DIRECTORY)) {
            wd = NULL;
            break;
        }

        //If wd points to a mountpoint then decend it
        if(wd->inode->flags & INODE_FLAG_MOUNTPOINT) {
            path_t mnt;
            mnt.mount = mount;
            mnt.dentry = wd;

            mount_t *submount = get_mount(&mnt);
            BUG_ON(!submount);

            mount = submount;
            wd = submount->fs->root;

            goto lookup_next;
        }

        //determine the name of the next working directory
        uint32_t len = 0;
        while(*next != PATH_SEPARATOR && *next) {
            next++;
            len++;
        }

        //have we reached the last path segment?
        if(!*next) {
            finished = true;
        }

        *next++ = '\0';

        //was there a pointless "//" in the path? skip it
        if(!len) {
            goto lookup_next;
        }

        //if the current path segment == "." skip it
        //if the current path semgent == ".." go up the tree
        if(path[0] == '.') switch(len) {
            case 1: {
                goto lookup_next;
            }
            case 2: {
                if(path[1] == '.') {
                    if(wd->parent) {
                        wd = wd->parent;
                    } else if(wd->inode->flags & INODE_FLAG_MOUNTPOINT) {
                        BUG_ON(!mount->parent);
                        wd = mount->mountpoint;
                        mount = mount->parent;
                    } else if(mount == root_mount) {
                        //do nothing
                    } else {
                        BUG();
                    }

                    goto lookup_next;
                }
            }
        }

        //have we cached this child?
        dentry_t *child;
        HASHTABLE_FOR_EACH_COLLISION(str_to_key(path, len), child, wd->children, node) {
            if(!memcmp(child->name, path, len)) {
                wd = child;
                goto lookup_next;
            }
        }

        //request the fs driver resolves the path segment into a new dentry
        child = dentry_alloc(strdup(path));
        wd->inode->ops->lookup(wd->inode, child);

        //does the requested path segment resolve?
        if(!child->inode) {
            dentry_free(child);
            wd = NULL;
            break;
        }

        wd = child;

lookup_next:
        path = next;
    }

    if(wd) {
        out->dentry = wd;
        out->mount = mount;
    }

    kfree(new_path);

    return !!wd;
}

file_t * vfs_open_file(inode_t *inode) {
    file_t *file = file_alloc(inode->ops->file_ops);
    if(file) {
        file->ops->open(file, inode);
    }

    return file;
}

off_t vfs_seek(file_t *file, uint32_t off) {
    return file->ops->seek(file, off);
}

ssize_t vfs_read(file_t *file, void *buff, size_t bytes) {
    return file->ops->read(file, buff, bytes);
}

ssize_t vfs_write(file_t *file, void *buff, size_t bytes) {
    return file->ops->write(file, buff, bytes);
}

static void fs_add(fs_t *fs) {
    list_add(&fs->list, &fs->type->instances);
}

dentry_t * fs_create_nodev(fs_type_t *type, void (*fill)(fs_t *fs)) {
    fs_t *fs = fs_alloc(type);
    fill(fs);

    fs_add(fs);

    return fs->root;
}

dentry_t * fs_create_single(fs_type_t *type, void (*fill)(fs_t *fs)) {
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
    fs_cache = cache_create(sizeof(fs_t));
    mount_cache = cache_create(sizeof(mount_t));

    return 0;
}

static INITCALL vfs_root_mount() {
    kprintf("fs - mounting root fs");

    fs_t *rootfs = vfs_fs_create("ramfs", NULL);
    if(!rootfs) return 1;

    return !(root_mount = vfs_mount_create(rootfs));
}

core_initcall(vfs_init);
subsys_initcall(vfs_root_mount);
