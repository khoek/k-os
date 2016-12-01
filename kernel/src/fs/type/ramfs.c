#include "common/types.h"
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
    list_add_before(&c->list, &r->chunks);
    return c;
}

static chunk_t * chunk_find(record_t *r, ssize_t off, bool create) {
    chunk_t *c;
    LIST_FOR_EACH_ENTRY(c, &r->chunks, list) {
        if(off >= CHUNK_SIZE) {
            off -= CHUNK_SIZE;
            continue;
        }

        if(c->used == off && !create) {
            return NULL;
        }

        return c;
    }

    return create ? chunk_alloc(r) : NULL;
}

static ssize_t chunk_write(chunk_t *c, char *buff, ssize_t len, ssize_t off) {
    len = MIN(CHUNK_SIZE - off, len);
    memcpy(c->buff + off, buff, len);
    c->used = c->used + MAX(0, off - c->used + len);
    return len;
}

static ssize_t chunk_read(chunk_t *c, char *buff, ssize_t len, ssize_t off) {
    len = MIN(MAX(0, c->used - off), len);
    memcpy(buff, c->buff + off, len);
    return len;
}

static ssize_t record_write(record_t *r, char *buff, ssize_t len, ssize_t off) {
    ssize_t written = 0;
    while(written < len) {
        chunk_t *c = chunk_find(r, off + written, true);
        written += chunk_write(c, buff + written, len - written, (off + written) % CHUNK_SIZE);
    }
    return written;
}

static ssize_t record_read(record_t *r, char *buff, ssize_t len, ssize_t off) {
    ssize_t read = 0;
    while(read < len) {
        chunk_t *c = chunk_find(r, off + read, false);
        if(!c) break;
        read += chunk_read(c, buff + read, len - read, (off + read) % CHUNK_SIZE);
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
    file->private = inode->private;
}

static off_t ramfs_file_seek(file_t *file, off_t offset) {
    file->offset = offset;
    return file->offset;
}

static ssize_t ramfs_file_read(file_t *file, char *buff, size_t bytes) {
    record_t *r = file->private;
    if(!r) {
        return -1;
    }

    ssize_t ret = record_read(r, buff, bytes, file->offset);
    file->offset += ret;
    return ret;
}

static ssize_t ramfs_file_write(file_t *file, char *buff, size_t bytes) {
    record_t *r = file->private;
    if(!r) {
        return -1;
    }

    ssize_t ret = record_write(r, buff, bytes, file->offset);
    file->offset += ret;
    return ret;
}

static void ramfs_file_close(file_t *file) {
}

static file_ops_t ramfs_file_ops = {
    .open   = ramfs_file_open,
    .seek   = ramfs_file_seek,
    .read   = ramfs_file_read,
    .write  = ramfs_file_write,
    .close  = ramfs_file_close,
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
    new->inode->mode = mode;

    record_t *r = record_create();
    new->inode->private = r;

    return new;
}

static bool ramfs_inode_mkdir(inode_t *inode, dentry_t *new, uint32_t mode) {
    new->inode = inode_alloc(inode->fs, &ramfs_inode_ops);
    new->inode->flags = INODE_FLAG_DIRECTORY;
    new->inode->mode = mode;

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
