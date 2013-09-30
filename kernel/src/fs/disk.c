#include "lib/int.h"
#include "lib/printf.h"
#include "common/list.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "fs/block.h"
#include "fs/subblock.h"
#include "fs/disk.h"
#include "fs/type/devfs.h"
#include "video/log.h"

static DEFINE_LIST(disk_labels);
static DEFINE_SPINLOCK(disk_label_lock);

void register_disk_label(disk_label_t *disk_label) {
    uint32_t flags;
    spin_lock_irqsave(&disk_label_lock, &flags);

    list_add(&disk_label->list, &disk_labels);

    spin_unlock_irqstore(&disk_label_lock, flags);
}

void register_disk(block_device_t *device) {
    //FIXME after implementing an IO scheduler
    //uint32_t flags;
    //spin_lock_irqsave(&disk_label_lock, &flags);

    disk_label_t *disk_label;
    LIST_FOR_EACH_ENTRY(disk_label, &disk_labels, list) {
        if(disk_label->probe(device)) {
            break;
        }
    }

    //spin_unlock_irqstore(&disk_label_lock, flags);
}

static uint8_t num_digits(uint8_t number) {
    int digits = 0;
    while(number) {
        number /= 10;
        digits++;
    }
    return digits;
}

void register_partition(block_device_t *device, uint8_t number, uint32_t start, uint32_t size) {
    uint32_t device_name_len = strlen(device->dentry->name);
    char *part_name = kmalloc(device_name_len + num_digits(number) + (isdigit(device->dentry->name[device_name_len - 1]) ? 2 : 1));
    sprintf(part_name, "%s%s%u", device->dentry->name, isdigit(device->dentry->name[device_name_len - 1]) ? "p" : "", number);

    devfs_add_device(subblock_device_open(device, start, size), part_name);
}
