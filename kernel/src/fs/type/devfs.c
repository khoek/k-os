#include "fs/vfs.h"
#include "fs/type/devfs.h"
#include "video/log.h"

void devfs_add_device(block_device_t *device, char *name) {
    logf("devfs - added /dev/%s", name);

    device->dentry = dentry_alloc(name);

    //TODO actually add this to /dev
}
