#ifndef KERNEL_MODULE_H
#define KERNEL_MODULE_H

#include <stdint.h>
#include "multiboot.h"

uint32_t module_count();
multiboot_module_t * module_get(uint32_t num);

void module_init(multiboot_info_t *mbd);

#endif
