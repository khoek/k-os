#include <stddef.h>

#include "common.h"
#include "init.h"
#include "cache.h"
#include "panic.h"
#include "log.h"
#include "mm.h"

//Cache includes
#include "task.h"

#define FREELIST_END (1 << 31)

#define CACHE_TASK_SIZE sizeof(task_t)

typedef struct cache_page cache_page_t;

struct cache_page {
    cache_page_t *prev, *next;
    uint32_t left;
    uint32_t free;
    uint32_t *freelist;
    void *mem;
    page_t *page;
};

typedef struct cache {
    uint32_t size;
    uint32_t max; // while not neccessary it might be good for stats/debugging or something
    cache_page_t *full;
    cache_page_t *partial;
    cache_page_t *empty;
} cache_t;

cache_t caches[NUM_CACHES];

static void cache_alloc_page(uint32_t cache) {
    //assume that there are no other empty cache pages
    page_t *page = alloc_page(0);
    caches[cache].empty = (cache_page_t *) page_to_virt(page);
    caches[cache].empty->page = page;

    caches[cache].empty->next = NULL;
    caches[cache].empty->prev = NULL;
    caches[cache].empty->left = caches[cache].max;

    if(!caches[cache].empty->left) panicf("0 objects left in a new cache page!");

    caches[cache].empty->free = 0;
    caches[cache].empty->freelist = (uint32_t *) (((uint8_t *) caches[cache].empty) + sizeof(cache_page_t));
    for(uint32_t i = 0; i < caches[cache].empty->left; i++) {
        caches[cache].empty->freelist[i] = i + 1;
    }
    caches[cache].empty->freelist[caches[cache].empty->left - 1] = FREELIST_END;

    caches[cache].empty->mem = caches[cache].empty->freelist + caches[cache].empty->left;
}

static void cache_create(uint32_t cache, uint32_t size) {
    caches[cache].size = size;
    caches[cache].max = (PAGE_SIZE - sizeof(cache_page_t)) / (caches[cache].size + sizeof(uint32_t));
    cache_alloc_page(cache);
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

void * cache_alloc(uint32_t cache) {
    if(cache >= NUM_CACHES) panicf("Illegal cache %u", cache);

    if(caches[cache].partial == NULL && caches[cache].empty == NULL) cache_alloc_page(cache);

    void *alloced = NULL;
    if(caches[cache].partial != NULL) {
        alloced = ((uint8_t *) caches[cache].partial->mem) + (caches[cache].partial->free * caches[cache].size);
        cache_do_alloc(caches[cache].partial);
        if(!caches[cache].partial->left) {
            cache_pop(&caches[cache].partial, &caches[cache].full);
        }
    } else if(caches[cache].empty != NULL) {
        alloced = ((uint8_t *) caches[cache].empty->mem) + (caches[cache].empty->free * caches[cache].size);
        cache_do_alloc(caches[cache].empty);
        if(caches[cache].empty->left == 0) {
            cache_pop(&caches[cache].empty, &caches[cache].full);
        } else {
            cache_pop(&caches[cache].empty, &caches[cache].partial);
        }
    } else {
        panicf("cache_alloc_page() failed to alloc new cache_page_t");
    }

    return alloced;
}

void cache_free(uint32_t cache, void *mem) {
    if(cache >= NUM_CACHES) panicf("Illegal cache %u", cache);

    cache_page_t *cur;
    for(cur = caches[cache].full; cur != NULL; cur = cur->next) {
        uint32_t addr = (uint32_t) page_to_virt(cur->page);
        if((addr >= ((uint32_t) mem)) && (((uint32_t) mem) < addr)) {
            break;
        }
    }

    if(cur == NULL) {
        for(cur = caches[cache].partial; cur != NULL; cur = cur->next) {
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

    if(cache_page->left + 1 == caches[cache].max) {
        cache_move(cache_page, &caches[cache].partial, &caches[cache].empty);
    } else if(!cache_page->left) {
        cache_move(cache_page, &caches[cache].full, &caches[cache].partial);
    }

    cache_do_free(cache_page, (((uint32_t) mem) - ((uint32_t)cache_page->mem)) / caches[cache].size);
}

//indirect, invoked from mm_init()
INITCALL cache_init() {
    cache_create(CACHE_TASK, CACHE_TASK_SIZE);

    logf("cache - created %u new object cache(s)", NUM_CACHES);
    return 0;
}
