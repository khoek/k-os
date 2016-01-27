#include <stddef.h>

#include "common/math.h"
#include "common/list.h"
#include "init/initcall.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "log/log.h"

#define FREELIST_END ((uint32_t) (1 << 31))

#define CACHE_FLAG_PERM (1 << 0)

typedef struct cache_page {
    cache_t *cache;
    list_head_t list;
    uint32_t left;
    uint32_t free;
    uint32_t *freelist;
    void *mem;
    page_t *page;
} cache_page_t;

struct cache {
    list_head_t list;
    uint32_t size;

    //while not neccessary it might be good for stats/debugging or something
    uint32_t max;

    uint32_t flags;
    spinlock_t lock;
    list_head_t full;
    list_head_t partial;
    list_head_t empty;
};

static cache_t meta_cache = {
    .size = sizeof(cache_t),
    .max = (PAGE_SIZE - sizeof(cache_page_t)) / (sizeof(cache_t) + sizeof(uint32_t)),
    .flags = CACHE_FLAG_PERM,
    .lock = SPINLOCK_UNLOCKED,
    .full = LIST_HEAD(meta_cache.full),
    .partial = LIST_HEAD(meta_cache.partial),
    .empty = LIST_HEAD(meta_cache.empty)
};

static DEFINE_LIST(caches);

static void cache_alloc_page(cache_t *cache) {
    page_t *page = alloc_page(ALLOC_CACHE);
    cache_page_t *cache_page = page_to_virt(page);
    cache_page->cache = cache;
    cache_page->page = page;
    cache_page->left = cache->max;

    list_add(&cache_page->list, &cache->empty);

    BUG_ON(!cache_page->left); //0 objects left in a new cache page!

    cache_page->free = 0;
    cache_page->freelist = (uint32_t *) (((uint8_t *) cache_page) + sizeof(cache_page_t));

    for(uint32_t i = 0; i < cache_page->left; i++) {
        cache_page->freelist[i] = i + 1;
    }

    cache_page->freelist[cache_page->left - 1] = FREELIST_END;

    cache_page->mem = cache_page->freelist + cache_page->left;
}

static void cache_do_alloc(cache_page_t *cache_page) {
    BUG_ON(cache_page->free == FREELIST_END);

    cache_page->left--;
    cache_page->free = cache_page->freelist[cache_page->free];
}

static void cache_do_free(cache_page_t *cache_page, uint32_t free_idx) {
    cache_page->left++;
    cache_page->freelist[free_idx] = cache_page->free;
    cache_page->free = free_idx;
}

void * cache_alloc(cache_t *cache) {
    uint32_t flags;
    spin_lock_irqsave(&cache->lock, &flags);

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

    spin_unlock_irqstore(&cache->lock, flags);

    return alloced;
}

static inline cache_page_t * mem_to_cache_page(void *mem) {
    return (void *) ((((uint32_t) mem) / PAGE_SIZE) * PAGE_SIZE);
}

void cache_free(cache_t *cache, void *mem) {
    uint32_t flags;
    spin_lock_irqsave(&cache->lock, &flags);

#ifdef CONFIG_DEBUG_MM
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
#else
    cache_page_t *target = mem_to_cache_page(mem);
#endif

    if(target->left + 1 == cache->max) {
        list_rm(&target->list);
        list_add(&target->list, &cache->empty);
    } else if(!target->left) {
        list_rm(&target->list);
        list_add(&target->list, &cache->partial);
    }

    cache_do_free(target, (((uint32_t) mem) - ((uint32_t) target->mem)) / cache->size);

    spin_unlock_irqstore(&cache->lock, flags);
}

cache_t * cache_create(uint32_t size) {
    cache_t *new = (cache_t *) cache_alloc(&meta_cache);
    list_add(&new->list, &caches);

    new->size = size;
    new->max = (PAGE_SIZE - sizeof(cache_page_t)) / (size + sizeof(uint32_t));
    new->flags = 0;

    spinlock_init(&new->lock);

    list_init(&new->empty);
    list_init(&new->partial);
    list_init(&new->full);

    return new;
}

static cache_t *kalloc_cache[KALLOC_NUM_CACHES];

static inline uint32_t kalloc_cache_index(uint32_t size) {
    for(uint32_t i = KALLOC_CACHE_SHIFT_MIN; i <= KALLOC_CACHE_SHIFT_MAX; i++) {
        if(size <= (1ULL << i)) {
            return i - KALLOC_CACHE_SHIFT_MIN;
        }
    }

    BUG();
}

void * kalloc_cache_alloc(uint32_t size) {
    if(size > KALLOC_CACHE_MAX) {
        panicf("cache_alloc() request too large: %u", size);
    }

    return cache_alloc(kalloc_cache[kalloc_cache_index(size)]);
}

void kalloc_cache_free(void *mem) {
    cache_free(mem_to_cache_page(mem)->cache, mem);
}

void __init cache_init() {
    list_add(&meta_cache.list, &caches);

    for(uint32_t i = 0; i < KALLOC_NUM_CACHES; i++) {
        kalloc_cache[i] = cache_create(1 << (i + KALLOC_CACHE_SHIFT_MIN));
    }

    kprintf("cache - metacache and kmalloc caches initialized");
}
