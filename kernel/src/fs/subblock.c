#include "lib/int.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "fs/vfs.h"
#include "fs/block.h"
#include "fs/subblock.h"

typedef struct subblock_data {
    block_device_t *parent;
    uint32_t start;
} subblock_data_t;

static ssize_t subblock_read(block_device_t *device, void *buff, size_t block, size_t count) {
    subblock_data_t *data = device->private;

    if(block >= device->size) return -1;

    return data->parent->ops->read(data->parent, buff, block + data->start, count);
}

static ssize_t subblock_write(block_device_t *device, void *buff, size_t block, size_t count) {
    subblock_data_t *data = device->private;

    if(block >= device->size) return -1;

    return data->parent->ops->write(data->parent, buff, block + data->start, count);
}

static block_device_ops_t subblock_ops = {
    .read  = subblock_read,
    .write = subblock_write,
};

block_device_t * subblock_device_open(block_device_t *parent, uint32_t start,  uint32_t size) {
    subblock_data_t *data = kmalloc(sizeof(subblock_data_t));
    data->parent = parent;
    data->start = start;

    block_device_t *sub = block_device_alloc();
    sub->private = data;
    sub->ops = &subblock_ops;
    sub->size = size;
    sub->block_size = parent->block_size;

    return sub;
}
