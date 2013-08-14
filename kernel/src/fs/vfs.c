#include "common/init.h"
#include "lib/int.h"
#include "common/list.h"
#include "common/listener.h"
#include "sync/spinlock.h"
#include "fs/vfs.h"

static file_t root_file = {
    .flags = FILE_FLAG_STATIC,

    .ops = NULL,
};

static dentry_t root = {
    .filename = "",
    .file = &root_file,

    .parent = NULL,
    .children = LIST_HEAD(root.children),
};

static DEFINE_LIST(disks);
static DEFINE_SPINLOCK(disk_lock);

void register_disk(disk_t *disk) {
    uint32_t flags;
    spin_lock_irqsave(&disk_lock, &flags);

    list_add(&disk->list, &disks);

    spin_unlock_irqstore(&disk_lock, flags);
}

void unregister_disk(disk_t *disk) {
    uint32_t flags;
    spin_lock_irqsave(&disk_lock, &flags);

    list_rm(&disk->list);

    spin_unlock_irqstore(&disk_lock, flags);
}

static INITCALL vfs_init() {
    return 0;
}

subsys_initcall(vfs_init);
