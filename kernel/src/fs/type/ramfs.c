#include "common/types.h"
#include "bug/debug.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "mm/cache.h"
#include "fs/vfs.h"
#include "log/log.h"

#define CHUNK_SIZE 512

typedef struct chunk {
    int32_t used;
    char *buff;

    list_head_t list;
} chunk_t;

typedef struct record {
    list_head_t chunks;
} record_t;

static cache_t *record_cache;
static cache_t *chunk_cache;
static cache_t *chunk_buff_cache;

static chunk_t * chunk_alloc(record_t *r) {
    chunk_t *c = cache_alloc(chunk_cache);
    c->used = 0;
    c->buff = cache_alloc(chunk_buff_cache);

    //FIXME is this the right call?
    list_add_before(&c->list, &r->chunks);
    return c;
}

static chunk_t * chunk_find(record_t *r, ssize_t off) {
    chunk_t *c;
    LIST_FOR_EACH_ENTRY(c, &r->chunks, list) {
        if(off >= CHUNK_SIZE) {
            off -= CHUNK_SIZE;
            continue;
        }

        return c;
    }

    return NULL;
}

static ssize_t chunk_write(chunk_t *c, const char *buff, ssize_t len, ssize_t off) {
    BUG_ON(off >= CHUNK_SIZE);
    len = MIN(CHUNK_SIZE - off, len);
    memcpy(c->buff + off, buff, len);
    c->used = MAX(c->used, off + len);
    return len;
}

static ssize_t chunk_read(chunk_t *c, char *buff, ssize_t len, ssize_t off) {
    BUG_ON(off > c->used);
    len = MIN(c->used - off, len);
    memcpy(buff, c->buff + off, len);
    return len;
}

static ssize_t record_write(inode_t *inode, const char *buff, ssize_t len, ssize_t off) {
    record_t *r = inode->private;

    ssize_t written = 0;
    while(written < len) {
        //FIXME this call to chunk_find is insane:
        //we should just find the first chunk we need, and keep taking the next
        //in the list. Probably, the list_add_before in chunk_alloc is wrong for
        //these purposes (figure out which of list_add and list_add_before puts
        //the entry at the end of the list).
        chunk_t *c = chunk_find(r, off + written);
        if(!c) {
            c = chunk_alloc(r);
        }
        uint32_t old_used = c->used;
        written += chunk_write(c, buff + written, len - written, (off + written) % CHUNK_SIZE);
        inode->size += c->used - old_used;
    }
    return written;
}

static ssize_t record_read(inode_t *inode, char *buff, ssize_t len, ssize_t off) {
    record_t *r = inode->private;

    ssize_t read = 0;
    while(read < len) {
        //FIXME this call to chunk_find is insane:
        //we should just find the first chunk we need, and keep taking the next
        //in the list. Probably, the list_add_before in chunk_alloc is wrong for
        //these purposes (figure out which of list_add and list_add_before puts
        //the entry at the end of the list).
        chunk_t *c = chunk_find(r, off + read);
        if(!c) {
            break;
        }

        uint32_t amt = chunk_read(c, buff + read, len - read, (off + read) % CHUNK_SIZE);
        //if we didn't read to the end of the chunk, but is has no more data,
        //i.e. we found the last chunk
        if(!amt) {
            break;
        }
        read += amt;
    }
    return read;
}

static record_t * record_create() {
    record_t *r = cache_alloc(record_cache);
    list_init(&r->chunks);
    chunk_alloc(r);
    return r;
}

static void ramfs_file_open(file_t *file, inode_t *inode) {
}

static void ramfs_file_close(file_t *file) {
}

static off_t ramfs_file_seek(file_t *file, off_t offset) {
    file->offset = offset;
    return file->offset;
}

static ssize_t ramfs_file_read(file_t *file, char *buff, size_t bytes) {
    ssize_t ret = record_read(file->dentry->inode, buff, bytes, file->offset);
    file->offset += ret;
    return ret;
}

static ssize_t ramfs_file_write(file_t *file, const char *buff, size_t bytes) {
    ssize_t ret = record_write(file->dentry->inode, buff, bytes, file->offset);
    file->offset += ret;
    return ret;
}

static file_ops_t ramfs_file_ops = {
    .open   = ramfs_file_open,
    .close  = ramfs_file_close,
    .seek   = ramfs_file_seek,
    .read   = ramfs_file_read,
    .write  = ramfs_file_write,

    .iterate = simple_file_iterate,
};

static void ramfs_inode_lookup(inode_t *inode, dentry_t *dentry);
static bool ramfs_inode_create(inode_t *inode, dentry_t *new, uint32_t mode);
static bool ramfs_inode_mkdir(inode_t *inode, dentry_t *new, uint32_t mode);

static inode_ops_t ramfs_inode_ops = {
    .file_ops = &ramfs_file_ops,

    .lookup = ramfs_inode_lookup,
    .create = ramfs_inode_create,
    .mkdir  = ramfs_inode_mkdir,
};

static void ramfs_inode_lookup(inode_t *inode, dentry_t *dentry) {
    dentry->inode = NULL;
}

static bool ramfs_inode_create(inode_t *inode, dentry_t *new, uint32_t mode) {
    new->inode = inode_alloc(inode->fs, &ramfs_inode_ops);
    new->inode->flags = 0;

    if(!S_ISREG(mode)) {
        return false;
    }
    new->inode->mode = mode;

    //FIXME sensibly populate these
    new->inode->uid = 0;
    new->inode->gid = 0;
    new->inode->nlink = 0;
    new->inode->dev = 0;
    new->inode->rdev = 0;

    new->inode->atime = 0;
    new->inode->mtime = 0;
    new->inode->ctime = 0;

    new->inode->blkshift = 12;
    new->inode->blocks = 8;

    new->inode->size = 0;

    new->inode->private = record_create();

    return new;
}

static bool ramfs_inode_mkdir(inode_t *inode, dentry_t *new, uint32_t mode) {
    new->inode = inode_alloc(inode->fs, &ramfs_inode_ops);
    new->inode->flags = INODE_FLAG_DIRECTORY;

    if(!S_ISDIR(mode)) {
        return false;
    }
    new->inode->mode = mode;

    //FIXME sensibly populate these
    new->inode->uid = 0;
    new->inode->gid = 0;
    new->inode->nlink = 0;
    new->inode->dev = 0;
    new->inode->rdev = 0;

    new->inode->atime = 0;
    new->inode->mtime = 0;
    new->inode->ctime = 0;

    new->inode->blkshift = 12;
    new->inode->blocks = 8;

    new->inode->size = 1 << new->inode->blkshift;

    new->inode->private = NULL;

    return true;
}

static dentry_t * ramfs_create(fs_type_t *type, const char *device);

static fs_type_t ramfs = {
    .name  = "ramfs",
    .flags = FSTYPE_FLAG_NODEV,
    .create  = ramfs_create,
};

static void ramfs_fill(fs_t *fs) {
    dentry_t *root = fs->root = dentry_alloc("");
    root->parent = NULL;
    root->fs = fs;

    root->inode = inode_alloc(fs, &ramfs_inode_ops);
    root->inode->flags |= INODE_FLAG_DIRECTORY;
    root->inode->mode = 0755;
}

static dentry_t * ramfs_create(fs_type_t *type, const char *device) {
    return fs_create_nodev(type, ramfs_fill);
}

static INITCALL ramfs_init() {
    record_cache = cache_create(sizeof(record_t));
    chunk_cache = cache_create(sizeof(chunk_t));
    chunk_buff_cache = cache_create(CHUNK_SIZE);

    register_fs_type(&ramfs);

    return 0;
}

core_initcall(ramfs_init);
