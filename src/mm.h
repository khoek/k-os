#include "multiboot.h"

#define PAGE_SIZE 4096

#define ALLOC_NONE 0

typedef struct page {
    uint8_t flags;
    uint8_t order;
    struct page * prev;
    struct page * next;
} page_t;

void mm_init(multiboot_memory_map_t *mmap, uint32_t length, uint32_t highest_module);

page_t * alloc_page();
void free_page(page_t *page);

void * page_to_address(page_t *page);
