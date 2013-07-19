#ifndef KERNEL_BOOT_MODULE_H
#define KERNEL_BOOT_MODULE_H

#include "lib/int.h"
#include "boot/multiboot.h"

uint32_t module_count();
multiboot_module_t * module_get(uint32_t num);

#endif
