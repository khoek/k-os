#include "lib/int.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/math.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "fs/disk.h"
#include "video/log.h"

#define GPT_HEADER_SECTOR 1

#define GPT_SIGNATURE "EFI PART"

typedef uint8_t guid_t[16];

typedef struct gpt_header {
    uint8_t  sig[8];
    uint32_t rev;
    uint32_t size;
    uint32_t crc;
    uint32_t zero;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_lba;
    uint64_t last_lba;
    guid_t   guid;
    uint64_t part_lba;
    uint32_t part_count;
    uint32_t part_size;
    uint32_t part_crc;
} PACKED gpt_header_t;

typedef struct gpt_part {
    guid_t   type;
    guid_t   unique;
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t flags;
    char     name[];
} gpt_part_t;

static guid_t PART_TYPE_UNUSED = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

static guid_t PART_TYPE_UEFI = {
    0x28, 0x73, 0x2A, 0xC1,
    0x1F, 0xF8, 0xD2, 0x11,
    0x4B, 0xBA, 0x00, 0xA0,
    0xC9, 0x3E, 0xC9, 0x3B,
};

static guid_t PART_TYPE_LINUX_DATA = {
    0xA2, 0xA0, 0xD0, 0xEB,
    0xE5, 0xB9, 0x33, 0x44,
    0xC0, 0x87, 0x68, 0xB6,
    0xB7, 0x26, 0x99, 0xC7,
};

static guid_t PART_TYPE_LINUX_SWAP = {
    0x6D, 0xFD, 0x57, 0x06,
    0xAB, 0xA4, 0xC4, 0x43,
    0xE5, 0x84, 0x09, 0x33,
    0xC8, 0x4B, 0x4F, 0x4F,
};

static guid_t PART_TYPE_WINDOWS_DATA = {
    0xAF, 0x3D, 0xC6, 0x0F,
    0x83, 0x84, 0x72, 0x47,
    0x79, 0x8E, 0x3D, 0x69,
    0xD8, 0x47, 0x7D, 0xE4,
};

static bool gpt_probe(block_device_t *device) {
    if(device->block_size < sizeof(gpt_header_t)) {
        return false;
    }

    gpt_header_t *gpt = kmalloc(device->block_size);
    if(device->ops->read(device, gpt, GPT_HEADER_SECTOR, 1) != 1) {
        goto probe_header_fail;
    }

    if(memcmp(gpt->sig, GPT_SIGNATURE, sizeof(gpt->sig))) {
        goto probe_header_fail;
    }

    ssize_t sectors = DIV_UP(gpt->part_size * gpt->part_count, device->block_size);
    uint32_t pages = DIV_UP(sectors * device->block_size, PAGE_SIZE);
    page_t *start = alloc_pages(pages, 0);
    gpt_part_t *part = page_to_virt(start);
    if(device->ops->read(device, part, gpt->part_lba, sectors) != sectors) {
        goto probe_table_fail;
    }

    for(uint8_t i = 0; i < gpt->part_count; i++) {
        if( !memcmp(&part->type, PART_TYPE_UEFI        , sizeof(guid_t)) ||
            !memcmp(&part->type, PART_TYPE_LINUX_DATA  , sizeof(guid_t)) ||
            !memcmp(&part->type, PART_TYPE_WINDOWS_DATA, sizeof(guid_t))) {
            register_partition(device, i + 1, part->first_lba, part->last_lba - part->first_lba);
        } else if(!memcmp(&part->type, PART_TYPE_LINUX_SWAP, sizeof(guid_t))) {
            //TODO call register_swap() (not yet implemented)
        } else if(memcmp(&part->type, PART_TYPE_UNUSED, sizeof(guid_t))) {
            logf("gpt - unrecognised type guid: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                part->type[ 3], part->type[ 2],
                part->type[ 1], part->type[ 0],
                part->type[ 5], part->type[ 4],
                part->type[ 7], part->type[ 6],
                part->type[ 9], part->type[ 8],
                part->type[10], part->type[11],
                part->type[12], part->type[13],
                part->type[14], part->type[15]);
        }

        part = (void *) (((uint32_t) part) + gpt->part_size);
    }

    free_pages(start, pages);
    kfree(gpt, device->block_size);
    return true;

probe_table_fail:
    free_pages(start, pages);
probe_header_fail:
    kfree(gpt, device->block_size);
    return false;
}

static disk_label_t gpt_label = {
    .probe = gpt_probe,
};

static INITCALL gpt_label_init() {
    register_disk_label(&gpt_label);

    return 0;
}

core_initcall(gpt_label_init);
