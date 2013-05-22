#ifndef KERNEL_IDE_H
#define KERNEL_IDE_H

#include <stdbool.h>
#include <stdint.h>
#include "init.h"

#define IDE_STRINGS         1
#define IDE_STRING_MODEL    0

void __init ide_init(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4);

bool ide_device_is_present(uint8_t device);
int8_t ide_device_get_type(uint8_t device);
uint32_t ide_device_get_size(uint8_t device);
char * ide_device_get_string(uint8_t device, uint8_t string);

int32_t ide_read_sectors(uint8_t drive, uint64_t numsects, uint32_t lba, void * edi);

int32_t ide_write_sectors(uint8_t drive, uint64_t numsects, uint32_t lba, void * edi);
int32_t ide_write_sectors_same(uint8_t drive, uint64_t numsects, uint32_t lba, void * edi);

#endif
