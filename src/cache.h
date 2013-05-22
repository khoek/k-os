#ifndef KERNEL_CACHE_H
#define KERNEL_CACHE_H

#include <stdint.h>
#include "init.h"

#define NUM_CACHES 1

#define CACHE_TASK 0

void * cache_alloc(uint32_t cache);
void cache_free(uint32_t cache, void *mem);

//indirect, invoked from mm_init()
INITCALL cache_init();

#endif
