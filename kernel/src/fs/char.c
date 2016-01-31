#include "common/types.h"
#include "init/initcall.h"
#include "mm/cache.h"
#include "fs/char.h"
#include "fs/type/devfs.h"

static cache_t *char_device_cache;

char_device_t * char_device_alloc() {
    char_device_t *dev = cache_alloc(char_device_cache);
    spinlock_init(&dev->lock);
    return dev;
}

void register_char_device(char_device_t *device, char *name) {
    devfs_add_chardev(device, name);
}

static INITCALL char_device_init() {
    char_device_cache = cache_create(sizeof(char_device_t));
    return 0;
}

core_initcall(char_device_init);
