#include "common/compiler.h"
#include "mm/cache.h"
#include "fs/fd.h"
#include "fs/stream/char.h"

typedef struct char_stream {
    uint32_t size;
    uint32_t front;
    uint32_t back;

    char *buff;
} char_stream_t;

static void char_stream_close(gfd_t *gfd) {
    kfree(gfd->private, sizeof(char_stream_t));
}

static fd_ops_t char_stream_ops = {
    .close = char_stream_close,
};

gfd_idx_t char_stream_alloc(uint32_t size) {
    char_stream_t *c = kmalloc(sizeof(char_stream_t));
    c->size = 0;
    c->buff = kmalloc(size);
    c->front = 0;
    c->back = 0;

    return gfdt_add(0, &char_stream_ops, c);
}
