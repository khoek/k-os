#include "lib/int.h"
#include "init/initcall.h"
#include "mm/cache.h"
#include "fs/block.h"
#include "fs/type/devfs.h"

static cache_t *block_device_cache;

block_device_t * block_device_alloc() {
    return cache_alloc(block_device_cache);
}

void register_block_device(block_device_t *device, char *name) {
    devfs_add_device(device, name);
}

static INITCALL block_device_init() {
    block_device_cache = cache_create(sizeof(block_device_t));
    return 0;
}

core_initcall(block_device_init);
