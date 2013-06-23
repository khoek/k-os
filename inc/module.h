#ifndef KERNEL_MODULE_H
#define KERNEL_MODULE_H

#include "int.h"
#include "multiboot.h"

uint32_t module_count();
multiboot_module_t * module_get(uint32_t num);

#endif
