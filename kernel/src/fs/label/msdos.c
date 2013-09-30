#include "lib/int.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "mm/cache.h"
#include "fs/vfs.h"
#include "video/log.h"

#define NUM_PARTITIONS 4

#define MSDOS_MAGIC 0xAA55

#define AIX_MAGIC 0xC1D4C2C9

#define PART_STATUS_ACTIVE   0x80
#define PART_STATUS_INACTIVE 0x00

#define PART_TYPE_EMPTY 0x00
#define PART_TYPE_FAT12 0x01
#define PART_TYPE_FAT16 0x06
#define PART_TYPE_FAT32 0x0b
#define PART_TYPE_NTFS  0x07
#define PART_TYPE_GPT   0xee

typedef struct chs {
    uint8_t head;
    uint8_t sector;
    uint8_t cylinder;
} PACKED chs_t;

typedef struct mbr_part {
    uint8_t  status;
    chs_t    chs_first;
    uint8_t  type;
    chs_t    chs_last;
    uint32_t start;
    uint32_t length;
} PACKED mbr_part_t;

typedef struct mbr {
    uint8_t    code[440];
    uint32_t   sig;
    uint16_t   zero;
    mbr_part_t parts[NUM_PARTITIONS];
    uint16_t   magic;
} PACKED mbr_t;

static bool maybe_fat(mbr_t *mbr) {
    if(!(mbr->code[0] == 0xeb || mbr->code[0] == 0xe9)) {
        return false;
    }

    uint32_t sector_size = swap_uint16(*(uint16_t *) (mbr->code + 11));
    switch (sector_size) {
        case 512:
        case 1024:
        case 2048:
        case 4096:
            break;
        default:
            return false;
    }

    if(!(mbr->code[21] == 0xf0 || mbr->code[21] == 0xf8)) {
        return false;
    }

    return true;
}

bool msdos_probe(block_device_t *device) {
    if(device->block_size < sizeof(mbr_t)) {
        return false;
    }

    mbr_t *mbr = kmalloc(device->block_size);
    memset(mbr, 0, 512);

    if(device->ops->read(device, mbr, 0, 1) != 1) {
        goto probe_fail;
    }

    if(mbr->magic != MSDOS_MAGIC) {
        goto probe_fail;
    }

    uint8_t num_active = 0;
    for(uint8_t i = 0; i < NUM_PARTITIONS; i++) {
        if(mbr->parts[i].status == PART_STATUS_ACTIVE) {
            num_active++;
        } else if (mbr->parts[i].status != PART_STATUS_INACTIVE) {
            goto probe_fail;
        }
    }

    if(num_active == 0 && maybe_fat(mbr)) {
        goto probe_fail;
    }

    for(uint8_t i = 0; i < NUM_PARTITIONS; i++) {
        if(mbr->parts[i].type == PART_TYPE_GPT) {
            goto probe_fail;
        }
    }

    if(*((uint32_t *) mbr->code) == AIX_MAGIC) {
        goto probe_fail;
    }

    for(uint8_t i = 0; i < NUM_PARTITIONS; i++) {
        if(mbr->parts[i].type != PART_TYPE_EMPTY) {
            register_partition(device, mbr->parts[i].start);
        }
    }

    kfree(mbr, device->block_size);
    return true;

probe_fail:
    kfree(mbr, device->block_size);
    return false;
}

static disk_label_t msdos_label = {
    .probe = msdos_probe,
};

static INITCALL msdos_label_init() {
    register_disk_label(&msdos_label);

    return 0;
}

core_initcall(msdos_label_init);
