#include <stddef.h>

#include "math.h"
#include "common.h"
#include "list.h"
#include "init.h"
#include "cache.h"
#include "panic.h"
#include "log.h"
#include "mm.h"

#define FREELIST_END (1 << 31)

#define CACHE_FLAG_PERM (1 << 0)

#define KALLOC_SHIFT_MIN 3
#define KALLOC_SHIFT_MAX 12
#define KALLOC_CACHES (KALLOC_SHIFT_MAX + 1)

typedef struct cache_page cache_page_t;

struct cache_page {
    list_head_t list;
    uint32_t left;
    uint32_t free;
    uint32_t *freelist;
    void *mem;
    page_t *page;
};

struct cache {
    list_head_t list;
    uint32_t size;
    uint32_t max; //while not neccessary it might be good for stats/debugging or something
    uint32_t flags;
    list_head_t full;
    list_head_t partial;
    list_head_t empty;
};

cache_t meta_cache = {
    .size = sizeof(cache_t),
    .max = (PAGE_SIZE - sizeof(cache_page_t)) / (sizeof(cache_t) + sizeof(uint32_t)),
    .flags = CACHE_FLAG_PERM,
    .full = LIST_MAKE_HEAD(meta_cache.full),
    .partial = LIST_MAKE_HEAD(meta_cache.partial),
    .empty = LIST_MAKE_HEAD(meta_cache.empty)
};

LIST_HEAD(caches);

static void cache_alloc_page(cache_t *cache) {
    page_t *page = alloc_page(0);
    cache_page_t *cache_page = (cache_page_t *) page_to_virt(page);
    cache_page->page = page;
    cache_page->left = cache->max;

    list_add(&cache_page->list, &cache->empty);

    if(!cache_page->left) panicf("0 objects left in a new cache page!");

    cache_page->free = 0;
    cache_page->freelist = (uint32_t *) (((uint8_t *) cache_page) + sizeof(cache_page_t));
    for(uint32_t i = 0; i < cache_page->left; i++) {
        cache_page->freelist[i] = i + 1;
    }
    cache_page->freelist[cache_page->left - 1] = FREELIST_END;

    cache_page->mem = cache_page->freelist + cache_page->left;
}

static void cache_do_alloc(cache_page_t *cache_page) {
    cache_page->left--;
    cache_page->free = cache_page->freelist[cache_page->free];
}

static void cache_do_free(cache_page_t *cache_page, uint32_t free_idx) {
    cache_page->left++;
    cache_page->freelist[free_idx] = cache_page->free;
    cache_page->free = free_idx;
}

void * cache_alloc(cache_t *cache) {
    if(list_empty(&cache->partial) && list_empty(&cache->empty)) cache_alloc_page(cache);

    void *alloced = NULL;
    if(!list_empty(&cache->partial)) {
        cache_page_t *page = list_first(&cache->partial, cache_page_t, list);
        alloced = ((uint8_t *) page->mem) + (page->free * cache->size);
        cache_do_alloc(page);
        if(!page->left) {
            list_rm(&page->list);
            list_add(&page->list, &cache->full);
        }
    } else if(!list_empty(&cache->empty)) {
        cache_page_t *page = list_first(&cache->empty, cache_page_t, list);
        alloced = ((uint8_t *) page->mem) + (page->free * cache->size);
        cache_do_alloc(page);

        list_rm(&page->list);
        if(!page->left) {
            list_add(&page->list, &cache->full);
        } else {
            list_add(&page->list, &cache->partial);
        }
    } else {
        panicf("cache_alloc_page() failed to alloc a new page");
    }

    return alloced;
}

void cache_free(cache_t *cache, void *mem) {
    //FIXME this is absurdly slow, make it faster by an order of complexity somehow :P

    cache_page_t *cur, *target = NULL;
    LIST_FOR_EACH_ENTRY(cur, &cache->full, list) {
        uint32_t addr = (uint32_t) page_to_virt(cur->page);
        if((((uint32_t) mem) >= addr) && (((uint32_t) mem) < (addr + PAGE_SIZE))) {
            target = cur;
            break;
        }
    }

    if(target == NULL) {
        LIST_FOR_EACH_ENTRY(cur, &cache->partial, list) {
            uint32_t addr = (uint32_t) page_to_virt(cur->page);
            if((((uint32_t) mem) >= addr) && (((uint32_t) mem) < (addr + PAGE_SIZE))) {
                target = cur;
                break;
            };
        }
    }

    if(target == NULL) {
        panic("Illegal mem ptr passed to cache_free");
    }

    cache_page_t *cache_page = (cache_page_t *) page_to_virt(cur->page);

    if(cache_page->left + 1 == cache->max) {
        list_rm(&cache_page->list);
        list_add(&cache_page->list, &cache->empty);
    } else if(!cache_page->left) {
        list_rm(&cache_page->list);
        list_add(&cache_page->list, &cache->partial);
    }

    cache_do_free(cache_page, (((uint32_t) mem) - ((uint32_t) cache_page->mem)) / cache->size);
}

cache_t * cache_create(uint32_t size) {
    cache_t *new = (cache_t *) cache_alloc(&meta_cache);
    list_add(&new->list, &caches);

    new->size = size;
    new->max = (PAGE_SIZE - sizeof(cache_page_t)) / (size + sizeof(uint32_t));
    new->flags = 0;

    list_init(&new->empty);
    list_init(&new->partial);
    list_init(&new->full);

    return new;
}

static cache_t *kalloc_cache[KALLOC_CACHES];

static uint32_t kalloc_cache_index(uint32_t size) {
    for (uint32_t i = KALLOC_SHIFT_MIN; i < KALLOC_SHIFT_MAX; i++) {
        if (size <= (1ULL << i)) {
            return i;
        }
    }

    return 0;
}

void * kmalloc(uint32_t size) {
    uint32_t index = kalloc_cache_index(size);
    if(!index) return NULL;

    return cache_alloc(kalloc_cache[index]);
}

void kfree(void *mem, uint32_t size) {
    uint32_t index = kalloc_cache_index(size);
    if(!index) return;

    cache_free(kalloc_cache[index], mem);
}

//indirect, invoked from mm_init()
INITCALL cache_init() {
    list_add(&meta_cache.list, &caches);

    for(uint32_t i = KALLOC_SHIFT_MIN; i < KALLOC_SHIFT_MAX; i++) {
        kalloc_cache[i] = cache_create(1 << i);
    }

    logf("cache - metacache and kmalloc caches initialized");

    return 0;
}
