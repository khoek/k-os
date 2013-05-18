#include <stdint.h>

#define NUM_CACHES 1

#define CACHE_TASK 0

void cache_init();

void * cache_alloc(uint32_t cache);
void cache_free(uint32_t cache, void *mem);
