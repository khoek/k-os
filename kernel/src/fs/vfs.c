#include "common/init.h"
#include "fs/vfs.h"

static INITCALL vfs_init() {
    return 0;
}

subsys_initcall(vfs_init);
