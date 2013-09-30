#ifndef KERNEL_MM_CACHE_H
#define KERNEL_MM_CACHE_H

#include "lib/int.h"
#include "init/initcall.h"

typedef struct cache cache_t;

cache_t * cache_create(uint32_t size);
void * cache_alloc(cache_t *cache);
void cache_free(cache_t *cache, void *mem);

void * kmalloc(uint32_t size);
void kfree(void *mem, uint32_t size);

void cache_init();

#endif
