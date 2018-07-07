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

static uint32_t global_ino_count = 1;

static cache_t *dentry_cache;
static cache_t *inode_cache;
static cache_t *fs_cache;
static cache_t *mount_cache;

static DEFINE_HASHTABLE(fs_types, 5);
static DEFINE_SPINLOCK(fs_type_lock);

static DEFINE_SPINLOCK(global_ino_lock);

static DEFINE_HASHTABLE(mount_hashtable, log2(PAGE_SIZE / sizeof(hashtable_node_t)));
static DEFINE_SPINLOCK(mount_hashtable_lock);

dentry_t * dentry_alloc(const char *name) {
    dentry_t *new = cache_alloc(dentry_cache);
    new->name = name;
    new->inode = NULL;
    new->flags = 0;
    new->parent = 0;
    hashtable_init(new->children_tab);
    list_init(&new->children_list);

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

    uint32_t flags;
    spin_lock_irqsave(&global_ino_lock, &flags);
    new->ino = global_ino_count++;
    spin_unlock_irqstore(&global_ino_lock, flags);

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

    if(parent) {
        hashtable_add(str_to_key(child->name, strlen(child->name)), &child->node, parent->children_tab);
        list_add(&child->list, &parent->children_list);
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
    BUG_ON(root->parent);
    BUG_ON(root != root->fs->root);

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
        if(*path == DIRECTORY_SEPARATOR && *(path + 1)) seg = path + 1;
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

//sanitise the (newly-created) inode associated to this dentry
static bool validate_inode(dentry_t *dentry) {
    BUG_ON(!dentry);
    if(!dentry->inode->ino) {
        panicf("Bad inode, ino = 0 for %X->%X", dentry, dentry->inode);
        return false;
    }

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
        if(!new->inode || !validate_inode(new))  {
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
    if(!newdir->inode || !validate_inode(newdir))  {
        kfree(copy);
        return false;
    }

    dentry_activate(newdir, wd.dentry);

    return true;
}

bool vfs_lookup(const path_t *start, const char *orig_path, path_t *out) {
    char *path = strdup(orig_path);
    char *new_path = path;

    path_t cwd;

    if(*path != DIRECTORY_SEPARATOR && start) {
        cwd = *start;
    } else {
        if(*path == DIRECTORY_SEPARATOR) {
            path++;
        }

        cwd = MNT_ROOT(root_mount);
    }

    bool finished = false;
    while(!finished && *path && cwd.dentry) {
        char *next = path;

        BUG_ON(!cwd.dentry->inode);

        if(!(cwd.dentry->inode->flags & INODE_FLAG_DIRECTORY)) {
            cwd.dentry = NULL;
            break;
        }

        //If cwd.dentry points to a mountpoint then decend it
        if(cwd.dentry->inode->flags & INODE_FLAG_MOUNTPOINT) {
            mount_t *submount = get_mount(&cwd);
            BUG_ON(!submount);
            cwd = MNT_ROOT(submount);

            goto lookup_next;
        }

        //determine the name of the next working directory
        uint32_t len = 0;
        while(*next != DIRECTORY_SEPARATOR && *next) {
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
                    while(!cwd.dentry->parent && cwd.mount != root_mount) {
                        BUG_ON(!cwd.mount->parent);
                        cwd.dentry = cwd.mount->mountpoint;
                        cwd.mount = cwd.mount->parent;

                        BUG_ON(!(cwd.dentry->inode->flags & INODE_FLAG_MOUNTPOINT));
                    }

                    if(cwd.dentry->parent) {
                        cwd.dentry = cwd.dentry->parent;
                    } else {
                        //do nothing if we are at the root
                    }

                    goto lookup_next;
                }
            }
        }

        //have we cached this child?
        dentry_t *child;
        HASHTABLE_FOR_EACH_COLLISION(str_to_key(path, len), child, cwd.dentry->children_tab, node) {
            if(!memcmp(child->name, path, len)) {
                cwd.dentry = child;
                goto lookup_next;
            }
        }

        //request the fs driver resolves the path segment into a new dentry
        child = dentry_alloc(strdup(path));
        cwd.dentry->inode->ops->lookup(cwd.dentry->inode, child);

        //does the requested path segment resolve?
        if(!child->inode) {
            dentry_free(child);
            cwd.dentry = NULL;
            break;
        }

        cwd.dentry = child;

lookup_next:
        path = next;
    }

    if(cwd.dentry) {
        if(cwd.dentry->inode->flags & INODE_FLAG_MOUNTPOINT) {
            mount_t *submount = get_mount(&cwd);
            BUG_ON(!submount);
            cwd = MNT_ROOT(submount);
        }

        *out = cwd;
    }

    kfree(new_path);

    return cwd.dentry;
}

file_t * vfs_open_file(dentry_t *dentry) {
    file_t *file = file_alloc(dentry->inode->ops->file_ops);
    if(file) {
        file->dentry = dentry;
        file->ops->open(file, dentry->inode);
    }

    return file;
}

off_t vfs_seek(file_t *file, uint32_t off) {
    return file->ops->seek(file, off);
}

ssize_t vfs_read(file_t *file, void *buff, size_t bytes) {
    return file->ops->read(file, buff, bytes);
}

ssize_t vfs_write(file_t *file, const void *buff, size_t bytes) {
    return file->ops->write(file, buff, bytes);
}

uint32_t simple_file_iterate(file_t *file, dir_entry_dat_t *buff, uint32_t num) {
    uint32_t curpos = 0;
    uint32_t num_read = 0;
    dentry_t *child;
    LIST_FOR_EACH_ENTRY(child, &file->dentry->children_list, list) {
        if(num_read >= num) {
            break;
        }

        if(curpos >= file->offset) {
            buff[num_read].ino = child->inode->ino;
            buff[num_read].type = child->inode->mode & INODE_FLAG_DIRECTORY ? ENTRY_TYPE_DIR : ENTRY_TYPE_FILE;
            strcpy(buff[num_read].name, child->name);

            num_read++;
        }

        curpos++;
    }

    file->offset = curpos;
    return num_read;
}

bool fs_no_create(inode_t *inode, dentry_t *d, uint32_t mode) {
    return false;
}

bool fs_no_mkdir(inode_t *inode, dentry_t *d, uint32_t mode) {
    return false;
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

uint32_t vfs_iterate(file_t *file, dir_entry_dat_t *buff, uint32_t num) {
  if(!file->ops->iterate) return -1;
  return file->ops->iterate(file, buff, num);
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
