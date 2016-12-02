#ifndef KERNEL_MM_MM_H
#define KERNEL_MM_MM_H

#define PAGE_SIZE 0x1000
#define STACK_NUM_PAGES 4

#define MALLOC_NUM_PAGES DIV_UP(sizeof(page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE)

typedef struct page page_t;

#include "common/types.h"
#include "common/list.h"

static inline uint32_t get_index(page_t *page);
static inline void * page_to_virt(page_t *page);

#include "init/initcall.h"
#include "arch/mmu.h"

#define ALLOC_CACHE (1 << 0)
#define ALLOC_COMPOUND (1 << 1)
#define ALLOC_ZERO (1 << 2)

struct page {
    uint32_t addr;
    uint8_t flags;
    uint8_t order;
    uint32_t compound_num;
    list_head_t list;
};

extern page_t *pages;
extern __initdata uint32_t lowmem;

static inline void * page_to_virt(page_t *page) {
    return (void *) (page->addr);
}

extern page_t *pages;

static inline uint32_t get_index(page_t *page) {
    return (((uint32_t) page) - ((uint32_t) pages)) / (sizeof(page_t));
}

static inline page_t * phys_to_page(phys_addr_t addr) {
    return &pages[phys_to_pageidx(addr)];
}

page_t * alloc_page(uint32_t flags);
page_t * alloc_pages(uint32_t pages, uint32_t flags);

void free_page(page_t *page);
void free_pages(page_t *first, uint32_t count);

void mm_init();
void mm_postinit_reclaim();

void claim_pages(uint32_t idx, uint32_t num);

void * kmalloc(uint32_t size);
void kfree(void *mem);

#endif
