#ifndef KERNEL_COMMON_RINGBUFF_H
#define KERNEL_COMMON_RINGBUFF_H

#include "mm/mm.h"

//FIXME use the preprocessor to emit typedefs which encode the type of each
//particular ringbuff, so the type doesn't have to be passed to each call below.
//Have ringbuff types declared via DECLARE_RINGBUFF_TYPE(), and then in the
//internal functions cast to a generic version (ringbuff_head_t) so there
//is only one implementation.

typedef struct {
    //These should really all be (void *), but it is convenient not to have to
    //cast for arithmetic.
    uint32_t start;
    uint32_t end;
    //We want front == back to mean an empty buffer, so...
    //front points to the first byte which can be read.
    uint32_t front;
    //Back points to the first byte after the last byte of data (so that, when
    //there is no wrapping in the buffer, back - front is exactly the number of
    //used bytes.
    uint32_t back;
    //The wrapping rules are that front and back can both never point to end.
} ringbuff_head_t;

//The rules are that we can advance front all the way into back, but we can only
//advance back until there is one space from front (else we will overflow).

#define DECLARE_RINGBUFF_STORAGE(type, name) type name
#define DECLARE_RINGBUFF(name) ringbuff_head_t name

#define _RINGBUFF_STATIC_DEFINE(name, buff_name) \
DECLARE_RINGBUFF(name) = {                             \
  .start = (uint32_t) (buff_name),                     \
  .end = ((uint32_t) (buff_name)) + sizeof(buff_name), \
  .front = (uint32_t) (buff_name),                     \
  .back = (uint32_t) (buff_name),                      \
}

#define DEFINE_RINGBUFF_STORAGE(type, name, len) type name[(len) + 1]
#define DEFINE_RINGBUFF(name, buff_name) _RINGBUFF_STATIC_DEFINE(name, buff_name)

#define ringbuff_init(rb, len, type) _do_ringbuff_init(rb, len, sizeof(type))
static void _do_ringbuff_init(ringbuff_head_t *rb, uint32_t len, uint32_t typesize) {
    //we have to add 1 to len because we leave a 1 block space
    uint32_t buff = (uint32_t) kmalloc((len + 1) * typesize);
    rb->start = rb->front = rb->back = buff;
    rb->end = buff + ((len + 1) * typesize);
}

#define ringbuff_destroy(rb, type) _do_ringbuff_destroy(rb, sizeof(type))
static void _do_ringbuff_destroy(ringbuff_head_t *rb, uint32_t typesize) {
    kfree((void *) rb->start);
}

#define ringbuff_alloc(len, type) _do_ringbuff_alloc(len, sizeof(type))
static void * _do_ringbuff_alloc(uint32_t len, uint32_t typesize) {
    //FIXME make a ringbuff cache
    ringbuff_head_t *rb = kmalloc(sizeof(ringbuff_head_t));
    _do_ringbuff_init(rb, len, typesize);
    return rb;
}

#define ringbuff_free(rb) _do_ringbuff_free(rb)
static void _do_ringbuff_free(ringbuff_head_t *rb, uint32_t typesize) {
    _do_ringbuff_destroy(rb, typesize);
    kfree(rb);
}

static size_t _ringbuff_chunksize_read(ringbuff_head_t *rb) {
    if(rb->front <= rb->back) {
        return rb->back - rb->front;
    } else {
        return rb->end - rb->front;
    }
}

static size_t _ringbuff_chunksize_write(ringbuff_head_t *rb, uint32_t typesize) {
    if(rb->front <= rb->back) {
        return (rb->end - rb->back) - (rb->front == rb->start ? typesize : 0);
    } else {
        //note here we have rb->front > rb->back for sure
        return rb->front - rb->back - typesize;
    }
}

//'amt' is just a number of bytes for us, but we assume that
//(amt % sizeof(type)) == 0. Also, we expect no overflows here (amt should
//always be <= _ringbuff_chunk_len(rb)).
static void _ringbuff_advance_front(ringbuff_head_t *rb, uint32_t amt) {
    rb->front += amt;
    if(rb->front == rb->end) {
        rb->front = rb->start;
    }
    BUG_ON(rb->front >= rb->end);
}

static void _ringbuff_advance_back(ringbuff_head_t *rb, uint32_t amt) {
    rb->back += amt;
    if(rb->back == rb->end) {
        rb->back = rb->start;
    }
    BUG_ON(rb->back >= rb->end);
}

#define ringbuff_is_empty(rb, type) _ringbuff_is_empty(rb, sizeof(type))
static bool _ringbuff_is_empty(ringbuff_head_t *rb, uint32_t typesize) {
    return rb->front == rb->back;
}

#define ringbuff_is_full(rb, type) _ringbuff_is_full(rb, sizeof(type))
static bool _ringbuff_is_full(ringbuff_head_t *rb, uint32_t typesize) {
    return rb->front == rb->back + typesize;
}

#define ringbuff_space_used(rb, type) _ringbuff_space_used(rb, sizeof(type))
static size_t _ringbuff_space_used(ringbuff_head_t *rb, uint32_t typesize) {
    uint32_t bytes_left;
    if(rb->front <= rb->back) {
        bytes_left = rb->back - rb->front;
    } else {
        bytes_left = (rb->end - rb->front) + (rb->back - rb->start);
    }
    BUG_ON(bytes_left % typesize);
    return bytes_left / typesize;
}

#define ringbuff_space_left(rb, type) _ringbuff_space_left(rb, sizeof(type))
static size_t _ringbuff_space_left(ringbuff_head_t *rb, uint32_t typesize) {
    uint32_t bytes_left;
    if(rb->back < rb->front) {
        bytes_left = rb->front - rb->back;
    } else {
        bytes_left = (rb->end - rb->back) + (rb->front - rb->start);
    }
    BUG_ON(bytes_left % typesize);
    return (bytes_left / typesize) - 1;
}

static ssize_t _do_ringbuff_read_chunk(ringbuff_head_t *rb, void *buff, uint32_t len) {
    uint32_t chunk_len = _ringbuff_chunksize_read(rb);
    uint32_t amt = MIN(len, chunk_len);
    memcpy(buff, (void *) rb->front, amt);
    _ringbuff_advance_front(rb, amt);
    return amt;
}

#define ringbuff_read(rb, buff, len, type) _do_ringbuff_read(rb, buff, len * sizeof(type), sizeof(type))
static ssize_t _do_ringbuff_read(ringbuff_head_t *rb, void *buff, uint32_t len, uint32_t typesize) {
    uint32_t amt = _do_ringbuff_read_chunk(rb, buff, len);
    if(len > amt && !_ringbuff_is_empty(rb, typesize)) {
        amt += _do_ringbuff_read_chunk(rb, buff + amt, len - amt);
    }
    return amt;
}

static ssize_t _do_ringbuff_write_chunk(ringbuff_head_t *rb, void *buff, uint32_t len, uint32_t typesize) {
    uint32_t chunk_len = _ringbuff_chunksize_write(rb, typesize);
    uint32_t amt = MIN(len, chunk_len);
    memcpy((void *) rb->back, buff, amt);
    _ringbuff_advance_back(rb, amt);
    return amt;
}

#define ringbuff_write(rb, buff, len, type) _do_ringbuff_write(rb, buff, len * sizeof(type), sizeof(type))
static ssize_t _do_ringbuff_write(ringbuff_head_t *rb, void *buff, uint32_t len, uint32_t typesize) {
    uint32_t amt = _do_ringbuff_write_chunk(rb, buff, len, typesize);
    if(len > amt && !_ringbuff_is_full(rb, typesize)) {
        amt += _do_ringbuff_write_chunk(rb, buff + amt, len - amt, typesize);
    }
    return amt;
}

#endif
