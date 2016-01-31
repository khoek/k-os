#ifndef KERNEL_FS_DISK_H
#define KERNEL_FS_DISK_H

typedef struct disk_label disk_label_t;

#include "common/types.h"
#include "common/list.h"
#include "fs/block.h"

struct disk_label {
    bool (*probe)(block_device_t *device, char *name);

    list_head_t list;
};

void register_disk_label(disk_label_t *disk_label);
void register_disk(block_device_t *device, char *name);
void register_partition(block_device_t *device, char *name, uint8_t number, uint32_t start, uint32_t size);

#endif
