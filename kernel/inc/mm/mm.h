#ifndef KERNEL_MM_MM_H
#define KERNEL_MM_MM_H

#include <stdbool.h>

#include "lib/int.h"

typedef struct page page_t;

struct page {
    uint32_t addr;
    uint8_t flags;
    uint8_t order;
    page_t *prev;
    page_t *next;
};

#include "init/multiboot.h"
#include "task/task.h"

#define PAGE_SIZE 0x1000
#define ALLOC_NONE 0

void * mm_map(const void *phys);

void page_build_directory(uint32_t directory[]);

page_t * alloc_page(uint32_t flags);
page_t * alloc_pages(uint32_t pages, uint32_t flags);

void free_page(page_t *page);
void free_pages(page_t *first, uint32_t count);

void * page_to_phys(page_t *page);
void * page_to_virt(page_t *page);
page_t * phys_to_page(void *addr);
void * virt_to_phys(void *addr);

void * alloc_page_user(uint32_t flags, task_t *task, uint32_t vaddr);

void mm_init();
void mm_postinit_reclaim();

#endif
