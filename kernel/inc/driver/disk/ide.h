#ifndef KERNEL_DRIVER_DISK_IDE_H
#define KERNEL_DRIVER_DISK_IDE_H

#include <stdbool.h>

#include "lib/int.h"

#define IDE_STRINGS         1
#define IDE_STRING_MODEL    0

bool ide_device_is_present(uint8_t device);
int8_t ide_device_get_type(uint8_t device);
uint32_t ide_device_get_size(uint8_t device);
char * ide_device_get_string(uint8_t device, uint8_t string);
bool ide_device_is_dma_capable(uint8_t device);

int32_t ide_read_sectors(uint8_t drive, uint64_t numsects, uint32_t lba, void * edi);

int32_t ide_write_sectors(uint8_t drive, uint64_t numsects, uint32_t lba, void * edi);
int32_t ide_write_sectors_same(uint8_t drive, uint64_t numsects, uint32_t lba, void * edi);

#endif
