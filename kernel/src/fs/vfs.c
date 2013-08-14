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

static DEFINE_LIST(disk_types);
static DEFINE_SPINLOCK(disk_type_lock);

void register_disk(disk_t *disk) {
    uint32_t flags;
    spin_lock_irqsave(&disk_lock, &flags);

    list_add(&disk->list, &disks);

    spin_unlock_irqstore(&disk_lock, flags);
}

void register_disk_type(disk_type_t *disk_type) {
    uint32_t flags;
    spin_lock_irqsave(&disk_type_lock, &flags);

    list_add(&disk_type->list, &disk_types);

    spin_unlock_irqstore(&disk_type_lock, flags);
}

static INITCALL vfs_init() {
    disk_t *disk;
    LIST_FOR_EACH_ENTRY(disk, &disks, list) {
        disk_type_t *disk_type;
        LIST_FOR_EACH_ENTRY(disk_type, &disk_types, list) {
            if(disk_type->match(disk)) {
                disk->type = disk_type;
                if(disk_type->probe(disk)) {
                    break;
                } else {
                    disk->type = NULL;
                }
            }
        }
    }

    return 0;
}

subsys_initcall(vfs_init);
