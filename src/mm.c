#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "init.h"
#include "mm.h"
#include "panic.h"
#include "log.h"
#include "cache.h"
#include "module.h"

#define MAX_ORDER 10
#define ADDRESS_SPACE_SIZE 4294967296ULL

#define NUM_ENTRIES 1024

#define KERNEL_TABLES (NUM_ENTRIES / 4)

#define PAGE_FLAG_USED (1 << 0)
#define PAGE_FLAG_USER (1 << 1)

extern uint32_t image_start;
extern uint32_t image_end;

static uint32_t kernel_start;
static uint32_t kernel_end;
static uint32_t num_pages;
static uint32_t mem_start;
static uint32_t mem_end;

static uint32_t page_directory[1024] ALIGN(PAGE_SIZE);
static uint32_t kernel_page_tables[255][1024] ALIGN(PAGE_SIZE);
static page_t  *pages;
static page_t  *free_page_list[MAX_ORDER + 1];

static inline void invlpg(uint32_t addr)
{
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static uint32_t get_index(page_t * page) {
    return (((uint32_t) page) - ((uint32_t) pages)) / (sizeof(page_t));
}

static page_t * get_buddy(page_t * page) {
    uint32_t index = get_index(page);

    if(index > (num_pages - 1)) {
        return NULL;
    }

    if(DIV_DOWN(index, (1 << page->order)) % 2) {
        return page - (1 << page->order);
    } else {
        return page + (1 << page->order);
    }
}

static inline void page_set(page_t *page, int8_t flag) {
    page->flags |= flag;
}

static inline void page_unset(page_t *page, int8_t flag) {
    page->flags &= ~flag;
}

page_t * alloc_page(uint32_t UNUSED(flags)) {
    for (uint32_t order = 0; order < MAX_ORDER + 1; order++) {
        if(free_page_list[order] != NULL) {
            for (; order > 0; order--) {
                page_t * page = free_page_list[order];
                page->order--;

                page_t * buddy = get_buddy(page);

                free_page_list[order] = page->next;

                page->prev = NULL;

                if(buddy == NULL) {
                    page->next = free_page_list[order - 1];
                } else {
                    page->next = buddy;

                    buddy->order = page->order;
                    buddy->prev = page;
                    buddy->next = free_page_list[order - 1];
                }

                free_page_list[order - 1] = page;
            }

            page_t *alloced = free_page_list[0];
            page_set(alloced, PAGE_FLAG_USED);

            if(free_page_list[0]->next) {
                free_page_list[0]->next->prev = NULL;
            }

            free_page_list[0] = free_page_list[0]->next;

            return alloced;
        }
    }

    panic("OOM!");
}

void free_page(page_t *page) {
    ASSERT(!BIT_SET(page->flags, PAGE_FLAG_USED));

    page_unset(page, PAGE_FLAG_USED);

    page_t *buddy = get_buddy(page);
    if(buddy != NULL && !BIT_SET(buddy->flags, PAGE_FLAG_USED) && buddy->order == page->order) {
        if(buddy->prev) {
            buddy->prev->next = buddy->next;
        } else {
            free_page_list[buddy->order] = buddy->next;
        }

        if(buddy->next) {
            buddy->next->prev = buddy->prev;
        }

        uint32_t page_index = get_index(page);
        page_t *primary_page = &pages[page_index - (page_index % 2)];
        primary_page->order = page->order + 1;

        if(free_page_list[primary_page->order]) {
             free_page_list[primary_page->order]->prev = primary_page;
        }

        primary_page->prev = NULL;
        primary_page->next = free_page_list[primary_page->order];

        free_page_list[primary_page->order] = primary_page;
    } else {
        if(free_page_list[page->order]) {
             free_page_list[page->order]->prev = page;
        }

        page->prev = NULL;
        page->next = free_page_list[page->order];

        free_page_list[page->order] = page;
    }
}

void * page_to_address(page_t *page) {
    uint32_t idx = get_index(page);

    return (void *) ((uint32_t *) page_directory[idx / 1024])[idx % 1024];
}

#define PRESENT     (1 << 0)
#define WRITABLE    (1 << 1)
#define CPL_KERNEL  (0 << 2)
#define CPL_USER    (1 << 2)

void page_protect(page_t *page, bool protect) {
    if(protect) page_unset(page, PAGE_FLAG_USER);
    else        page_set(page, PAGE_FLAG_USER);

    uint32_t addr = (uint32_t) page_to_address(page);
    if(protect) ((uint32_t *) (page_directory[addr / (PAGE_SIZE * 1024)] & ~0xFFF))[(addr % (PAGE_SIZE * 1024)) / PAGE_SIZE] &= ~CPL_USER;
    else        ((uint32_t *) (page_directory[addr / (PAGE_SIZE * 1024)] & ~0xFFF))[(addr % (PAGE_SIZE * 1024)) / PAGE_SIZE] |= CPL_USER;

    invlpg(addr);
}

void page_build_directory() {

}

static void paging_init() {
    uint32_t original_start = mem_start;

    //reserve space for the page_t structs, remaining page aligned
    pages = (page_t *) mem_start;
    num_pages = DIV_DOWN(mem_end - mem_start, PAGE_SIZE);
    mem_start += DIV_UP(sizeof(page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE;

    logf("mm - total: %u MB, malloc %u MB, avaliable %u MB",
    DIV_DOWN(DIV_UP(sizeof(uint32_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
    DIV_DOWN(DIV_UP(sizeof(page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
    DIV_DOWN(mem_end - mem_start,       1024 * 1024));
    logf("mm - address space: (phys)0x%p/(avail)0x%p - 0x%p", original_start, mem_start, mem_end);

    free_page_list[MAX_ORDER] = &pages[0];

    page_t * last_page = NULL;
    for (uint32_t page = 0; page < num_pages; page++) {
        pages[page].flags = 0;
        pages[page].order = MAX_ORDER;

        pages[page].next = NULL;
        if (page % (1 << MAX_ORDER) == 0) {
             pages[page].prev = last_page;

             if (last_page != NULL) {
                 last_page->next = &pages[page];
             }

             last_page = &pages[page];
        } else {
             pages[page].prev = NULL;
        }
    }

    for(uint32_t i = 0; i < KERNEL_TABLES; i++) {
        for(uint32_t j = 0; j < NUM_ENTRIES; j++) {
            uint32_t phys = ((i * 1024) + j) * PAGE_SIZE;
            if(phys < kernel_end) {
                kernel_page_tables[i][j] = phys | WRITABLE | PRESENT;
            }
        }
    }

    for(uint32_t i = 0; i < KERNEL_TABLES; i++) {
        page_directory[i + (VIRTUAL_BASE / PAGE_SIZE / NUM_ENTRIES)] = (((uint32_t) &kernel_page_tables[i]) - VIRTUAL_BASE) | PRESENT | WRITABLE | CPL_USER;
    }

    __asm__ volatile("xchg %%bx, %%bx" ::);
    __asm__ volatile("mov %0, %%cr3":: "a" (((uint32_t) page_directory) - VIRTUAL_BASE));
}

static INITCALL mm_init() {
    kernel_start = ((uint32_t) &image_start);
    kernel_end = ((uint32_t) &image_end);

    for(uint32_t i = 0; i < module_count(); i++) {
        uint32_t end = module_get(i)->end - 1;
        if(kernel_end < end) {
            kernel_end = end;
        }
    }

    logf("mm - kernel image 0x%p-0x%p", kernel_start, kernel_end);

    multiboot_memory_map_t *mmap = multiboot_info->mmap;
    uint32_t best_len = 0;
    for(uint32_t i = 0; i < (multiboot_info->mmap_length / sizeof(multiboot_memory_map_t)); i++) {
        if(mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
             if(mmap[i].addr >= ADDRESS_SPACE_SIZE) {
                 continue;
             }

             uint32_t start = mmap[i].addr;
             uint64_t end = mmap[i].addr + mmap[i].len;

             if(end > ADDRESS_SPACE_SIZE) {
                end = ADDRESS_SPACE_SIZE - 1ULL;
             }

             if(start <= kernel_start && kernel_start < end) {
                if(kernel_end < end) {
                    if(kernel_start - start > end - kernel_end) {
                        end = kernel_start;
                    } else {
                        start = kernel_end;
                    }
                } else {
                    end = kernel_start;
                }
             } else if(start <= kernel_end && kernel_end < end) {
                start = kernel_end;
             } else if(kernel_start <= start && end <= kernel_end) {
                continue;
             }

             start = DIV_UP(start, PAGE_SIZE) * PAGE_SIZE;
             end = DIV_DOWN(end, PAGE_SIZE) * PAGE_SIZE;

             if(start >= end) {
                continue;
             }

             if(end - start > best_len) {
                best_len = end - start;
                mem_start = start;
                mem_end = end;
             }
        }
    }

    if(best_len == 0) panic("MM - did not find suitable memory region");

    paging_init();

    return cache_init();
}

core_initcall(mm_init);
