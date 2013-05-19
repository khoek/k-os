#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include "multiboot.h"

#define PAGE_SIZE 0x1000
#define ALLOC_NONE 0

typedef struct page page_t;

struct page {
    uint8_t flags;
    uint8_t order;
    page_t *prev;
    page_t *next;
    uint32_t cache; //Pointer for quick cache freeing
};

void mm_init(multiboot_info_t *mbd);

page_t * alloc_page();
void free_page(page_t *page);

void * page_to_address(page_t *page);
page_t * address_to_page(void *address);

#endif
