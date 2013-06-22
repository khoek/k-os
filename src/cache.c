#include <stddef.h>

#include "common.h"
#include "list.h"
#include "init.h"
#include "cache.h"
#include "panic.h"
#include "log.h"
#include "mm.h"

#define FREELIST_END (1 << 31)

#define CACHE_FLAG_PERM (1 << 0)

typedef struct cache_page cache_page_t;

struct cache_page {
    cache_page_t *prev, *next;
    uint32_t left;
    uint32_t free;
    uint32_t *freelist;
    void *mem;
    page_t *page;
};

struct cache {
    list_head_t list;
    uint32_t flags;
    uint32_t size;
    uint32_t max; // while not neccessary it might be good for stats/debugging or something
    cache_page_t *full;
    cache_page_t *partial;
    cache_page_t *empty;
};

cache_t meta_cache = {
    .size = sizeof(cache_t),
    .max = (PAGE_SIZE - sizeof(cache_page_t)) / (sizeof(cache_t) + sizeof(uint32_t)),
    .flags = CACHE_FLAG_PERM
};

LIST_HEAD(caches);

static void cache_alloc_page(cache_t *cache) {
    //assume that there are no other empty cache pages
    page_t *page = alloc_page(0);
    cache->empty = (cache_page_t *) page_to_virt(page);
    cache->empty->page = page;

    cache->empty->next = NULL;
    cache->empty->prev = NULL;
    cache->empty->left = cache->max;

    if(!cache->empty->left) panicf("0 objects left in a new cache page!");

    cache->empty->free = 0;
    cache->empty->freelist = (uint32_t *) (((uint8_t *) cache->empty) + sizeof(cache_page_t));
    for(uint32_t i = 0; i < cache->empty->left; i++) {
        cache->empty->freelist[i] = i + 1;
    }
    cache->empty->freelist[cache->empty->left - 1] = FREELIST_END;

    cache->empty->mem = cache->empty->freelist + cache->empty->left;
}

static void cache_pop(cache_page_t **s, cache_page_t **d) {
    cache_page_t *page = *s;

    *s = page->next;
    if(*s) {
        (*s)->prev = NULL;
    }

    page->next = *d;
    page->prev = NULL;

    if(*d) {
        (*d)->prev = page;
    }
    *d = page;
}

static void cache_move(cache_page_t *page, cache_page_t **s, cache_page_t **d) {
    if(page->prev) {
        page->prev->next = page->next;
    } else {
        *s = page->next;
    }

    if(page->next) {
        page->next->prev = page->prev;
    }

    page->next = *d;
    page->prev = NULL;

    if(*d) {
        (*d)->prev = page;
    }
    *d = page;
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
    if(cache->partial == NULL && cache->empty == NULL) cache_alloc_page(cache);

    void *alloced = NULL;
    if(cache->partial != NULL) {
        alloced = ((uint8_t *) cache->partial->mem) + (cache->partial->free * cache->size);
        cache_do_alloc(cache->partial);
        if(!cache->partial->left) {
            cache_pop(&cache->partial, &cache->full);
        }
    } else if(cache->empty != NULL) {
        alloced = ((uint8_t *) cache->empty->mem) + (cache->empty->free * cache->size);
        cache_do_alloc(cache->empty);
        if(cache->empty->left == 0) {
            cache_pop(&cache->empty, &cache->full);
        } else {
            cache_pop(&cache->empty, &cache->partial);
        }
    } else {
        panicf("cache_alloc_page() failed to alloc new cache_page_t");
    }

    return alloced;
}

void cache_free(cache_t *cache, void *mem) {
    cache_page_t *cur;
    for(cur = cache->full; cur != NULL; cur = cur->next) {
        uint32_t addr = (uint32_t) page_to_virt(cur->page);
        if((addr >= ((uint32_t) mem)) && (((uint32_t) mem) < addr)) {
            break;
        }
    }

    if(cur == NULL) {
        for(cur = cache->partial; cur != NULL; cur = cur->next) {
            uint32_t addr = (uint32_t) page_to_virt(cur->page);
            if((addr >= ((uint32_t) mem)) && (((uint32_t) mem) < addr)) {
                break;
            };
        }
    }

    if(cur == NULL) {
        panic("Illegal memzone param to cache_free");
    }

    cache_page_t *cache_page = (cache_page_t *) page_to_virt(cur->page);

    if(cache_page->left + 1 == cache->max) {
        cache_move(cache_page, &cache->partial, &cache->empty);
    } else if(!cache_page->left) {
        cache_move(cache_page, &cache->full, &cache->partial);
    }

    cache_do_free(cache_page, (((uint32_t) mem) - ((uint32_t)cache_page->mem)) / cache->size);
}

cache_t * cache_create(uint32_t size) {
    cache_t *new = (cache_t *) cache_alloc(&meta_cache);
    list_add(&new->list, &caches);

    new->size = size;
    new->max = (PAGE_SIZE - sizeof(cache_page_t)) / (size + sizeof(uint32_t));

    return new;
}

//indirect, invoked from mm_init()
INITCALL cache_init() {
    list_add(&meta_cache.list, &caches);

    logf("cache - metacache initialized");

    return 0;
}
