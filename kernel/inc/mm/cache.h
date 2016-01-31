#ifndef KERNEL_MM_CACHE_H
#define KERNEL_MM_CACHE_H

#define KALLOC_CACHE_SHIFT_MIN 3
#define KALLOC_CACHE_SHIFT_MAX 11

#define KALLOC_CACHE_MAX (1ULL << KALLOC_CACHE_SHIFT_MAX)
#define KALLOC_NUM_CACHES (KALLOC_CACHE_SHIFT_MAX - KALLOC_CACHE_SHIFT_MIN + 1)

#include "common/types.h"
#include "init/initcall.h"

typedef struct cache cache_t;

cache_t * cache_create(uint32_t size);
void * cache_alloc(cache_t *cache);
void cache_free(cache_t *cache, void *mem);

void * kalloc_cache_alloc(uint32_t size);
void kalloc_cache_free(void *mem);

void cache_init();

#endif
