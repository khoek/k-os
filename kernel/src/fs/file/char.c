#include "common/compiler.h"
#include "mm/cache.h"
#include "fs/fd.h"
#include "fs/vfs.h"
#include "fs/file/char.h"

typedef struct char_buff {
    uint32_t size;
    uint32_t front;

    char *buff;
} char_buff_t;

static ssize_t char_read(file_t *file, char *buff, size_t bytes) {
    char_buff_t *c = file->private;

    if(!bytes) return 0;

    if(c->front == file->offset)  {
        //TODO errno = something
        return -1;
    }

    bytes = MIN(bytes, c->front - file->offset);

    memcpy(buff + file->offset, c->buff, bytes);

    file->offset += bytes;

    return bytes;
}

static ssize_t char_write(file_t *file, char *buff, size_t bytes) {
    char_buff_t *c = file->private;

    if(!bytes) return 0;

    if(c->size == c->front)  {
        //TODO errno = something
        return -1;
    }

    bytes = MIN(bytes, c->size - c->front);

    memcpy(c->buff, buff, bytes);

    return bytes;
}

static void char_close(file_t *file) {
    kfree(file->private, sizeof(char_buff_t));
}

static file_ops_t char_file_ops = {
    .open = NULL,
    .seek = NULL,
    .read = char_read,
    .write = char_write,
    .close = char_close,
};

file_t * char_file_open(uint32_t size) {
    char_buff_t *c = kmalloc(sizeof(char_buff_t));
    c->size = size;
    c->buff = kmalloc(size);
    c->front = 0;

    file_t *new = file_alloc(&char_file_ops);
    new->private = c;

    return new;
}
