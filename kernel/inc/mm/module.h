#ifndef KERNEL_MM_MODULE_H
#define KERNEL_MM_MODULE_H

#include "init/initcall.h"

__init void module_init();
__init bool is_module_page(uint32_t idx);

__init void module_load();

#endif
