#include "common/types.h"
#include "bug/debug.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "mm/cache.h"
#include "fs/vfs.h"
#include "log/log.h"

#define CHUNK_SIZE 1024

typedef struct chunk {
    uint32_t used;
    void *buff;

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

//CONTRACT: if chunk_read and chunk_write return a size less than the size
//passed to them, then immediately calling chunk_read/write with the remainder
//must return zero (they return having filled/emptied the buffer as best they
//could). This might become important if the data structure underlying chunks
//is changed (and the get bigger).

static ssize_t chunk_read(chunk_t *c, void *buff, size_t len, size_t off) {
    BUG_ON(off > c->used);
    len = MIN(c->used - off, len);
    memcpy(buff, c->buff + off, len);
    return len;
}

//record_size is for bookkeeping of this record, and is updated when we expand
static ssize_t chunk_write(chunk_t *c, size_t *record_size, const void *buff,
    size_t len, size_t off) {
    BUG_ON(off >= CHUNK_SIZE);
    len = MIN(CHUNK_SIZE - off, len);
    memcpy(c->buff + off, buff, len);

    uint32_t old_used = c->used;
    c->used = MAX(c->used, off + len);
    *record_size += c->used - old_used;

    return len;
}

static ssize_t record_read(file_t *file, void *buff, size_t len, size_t off) {
    chunk_t *c = file->private; //current chunk
    inode_t *inode = file->dentry->inode;
    record_t *r = inode->private;
    ssize_t amt = 0;

    if(!c) {
        return 0;
    }

    while(len - amt && c) {
        amt += chunk_read(c, buff + amt, len - amt, (off + amt) % CHUNK_SIZE);

        if(!(len - amt) || list_is_last(c, &r->chunks, list)) {
            file->private = (off + amt) % CHUNK_SIZE ? c : list_next(c, &r->chunks, list);
        }
        c = list_next(c, &r->chunks, list);
    }

    file->offset += amt;
    return amt;
}

static ssize_t record_write(file_t *file, const void *buff, size_t len, size_t off) {
    chunk_t *c = file->private; //current chunk
    inode_t *inode = file->dentry->inode;
    record_t *r = inode->private;
    ssize_t amt = 0;

    while(len - amt) {
        if(!c) {
            c = chunk_alloc(r);
        }

        amt += chunk_write(c, &inode->size, buff + amt, len - amt,
            (off + amt) % CHUNK_SIZE);

        if(len - amt) {
            if(c) {
                c = list_next(c, &r->chunks, list);
            }
        } else {
            file->private = (off + amt) % CHUNK_SIZE ? c : list_next(c, &r->chunks, list);
        }
    }

    file->offset += amt;
    return amt;
}

static record_t * record_create() {
    record_t *r = cache_alloc(record_cache);
    list_init(&r->chunks);
    return r;
}

//Note: inode->private stores a record_t *, while file->private stores a
//chunk_t *.

//XXX if we ever implement deletion/deallocation of chunks, we must be mindful
//of other file_t *s storing pointers to chunks which we are deleting, or
//rearranging.

static void ramfs_file_open(file_t *file, inode_t *inode) {
    if(!(inode->flags & INODE_FLAG_DIRECTORY)) {
        record_t *r = inode->private;
        if(list_empty(&r->chunks)) {
            file->private = NULL;
        } else {
            file->private = list_first(&r->chunks, chunk_t, list);
        }
    }
}

static void ramfs_file_close(file_t *file) {
}

static off_t ramfs_file_seek(file_t *file, off_t offset, int whence) {
    if(!(whence == SEEK_SET)) {
        panicf("ramfs - unimplemented whence value %d", whence);
    }

    inode_t *inode = file->dentry->inode;
    record_t *r = inode->private;
    size_t pos = 0;

    //FIXME only do a relative seek, instead of just starting over
    chunk_t *c;
    LIST_FOR_EACH_ENTRY(c, &r->chunks, list) {
        if(offset - pos < CHUNK_SIZE) {
            if(offset - pos > c->used) {
                //FIXME allow seeking beyond end of file
                panic("ramfs - seek beyond EOF");
            }

            file->private = c;
            file->offset = offset;
            return offset;
        }
        pos += CHUNK_SIZE;
    }

    //FIXME allow seeking beyond end of file
    panic("ramfs - seek beyond EOF");
}

static ssize_t ramfs_file_read(file_t *file, char *buff, size_t bytes) {
    return record_read(file, buff, bytes, file->offset);
}

static ssize_t ramfs_file_write(file_t *file, const char *buff, size_t bytes) {
    return record_write(file, buff, bytes, file->offset);
}

static int32_t ramfs_file_poll(file_t *file, fpoll_data_t *fp) {
    fp->readable = true;
    fp->writable = true;
    fp->errored = false;
    return 0;
}

static file_ops_t ramfs_file_ops = {
    .open  = ramfs_file_open,
    .close = ramfs_file_close,
    .seek  = ramfs_file_seek,
    .read  = ramfs_file_read,
    .write = ramfs_file_write,
    .poll  = ramfs_file_poll,

    .iterate = simple_file_iterate,
};

static void ramfs_inode_lookup(inode_t *inode, dentry_t *dentry);
static int32_t ramfs_inode_create(inode_t *inode, dentry_t *new, uint32_t mode);

static inode_ops_t ramfs_inode_ops = {
    .file_ops = &ramfs_file_ops,

    .lookup = ramfs_inode_lookup,
    .create = ramfs_inode_create,
};

static void ramfs_inode_lookup(inode_t *inode, dentry_t *dentry) {
    dentry->inode = NULL;
}

static int32_t ramfs_create_internal(fs_t *fs, dentry_t *new, uint32_t mode) {
    if(S_ISREG(mode)) {
        new->inode = inode_alloc(fs, &ramfs_inode_ops);
        new->inode->mode = mode;
        new->inode->flags = 0;
        new->inode->size = 0;
        new->inode->private = record_create();
    } else if(S_ISDIR(mode)) {
        new->inode = inode_alloc(fs, &ramfs_inode_ops);
        new->inode->mode = mode;
        new->inode->flags = INODE_FLAG_DIRECTORY;
        new->inode->size = 1 << new->inode->blkshift;
        new->inode->private = NULL;
    } else {
        return -EINVAL;
    }

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

    return 0;
}

static int32_t ramfs_inode_create(inode_t *inode, dentry_t *d, uint32_t mode) {
    return ramfs_create_internal(inode->fs, d, mode);
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

    int32_t ret = ramfs_create_internal(fs, fs->root, S_IFDIR | 0755);
    if(ret) {
        panicf("ramfs_fill failed: %d", ret);
    }
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
