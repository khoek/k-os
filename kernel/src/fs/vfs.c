#include "lib/int.h"
#include "common/init.h"
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

static DEFINE_LIST(disk_labels);
static DEFINE_SPINLOCK(disk_label_lock);

void register_disk(disk_t *disk) {
    uint32_t flags;
    spin_lock_irqsave(&disk_lock, &flags);

    list_add(&disk->list, &disks);

    spin_unlock_irqstore(&disk_lock, flags);
}

void register_disk_label(disk_label_t *disk_label) {
    uint32_t flags;
    spin_lock_irqsave(&disk_label_lock, &flags);

    list_add(&disk_label->list, &disk_labels);

    spin_unlock_irqstore(&disk_label_lock, flags);
}

static INITCALL disk_recognise() {
    disk_t *disk;
    LIST_FOR_EACH_ENTRY(disk, &disks, list) {
        disk_label_t *disk_label;
        LIST_FOR_EACH_ENTRY(disk_label, &disk_labels, list) {
            disk->type = disk_label;

            if(disk_label->probe(disk)) {
                break;
            }

            disk->type = NULL;
        }
    }

    return 0;
}

device_initcall(disk_recognise);
