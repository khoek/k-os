#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <stdbool.h>
#include "multiboot.h"

#define PAGE_SIZE 0x1000
#define ALLOC_NONE 0

typedef struct page page_t;

struct page {
    uint32_t addr;
    uint8_t flags;
    uint8_t order;
    page_t *prev;
    page_t *next;
};

page_t * alloc_page(uint32_t flags);
page_t * alloc_page_user(uint32_t flags, uint32_t *dir);

void page_protect(page_t *page, bool protect);

void free_page(page_t *page);

void * page_to_phys(page_t *page);
void * page_to_virt(page_t *page);

#endif
