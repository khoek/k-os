#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "mm.h"
#include "common.h"
#include "panic.h"
#include "console.h"
#include "module.h"

#define MAX_ORDER 10
#define ADDRESS_SPACE_SIZE 4294967296ULL

#define NUM_ENTRIES 1024

#define PAGE_FLAG_USED (1 << 0)

extern uint32_t end_of_image;
static uint32_t kernel_end;

static uint32_t num_pages;
static uint32_t mem_start;
static uint32_t mem_end;

static uint32_t page_directory[1024] ALIGN(PAGE_SIZE);
static page_t  *pages;
static page_t  *free_page_list[MAX_ORDER + 1];

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

             page_t *alloc = free_page_list[0];
             free_page_list[0] = free_page_list[0]->next;
             page_set(alloc, PAGE_FLAG_USED);

             return alloc;
        }
    }

    return NULL;
}

void free_page(page_t *page) {
    page_unset(page, PAGE_FLAG_USED);

    page_t *buddy = get_buddy(page);
    if(buddy != NULL && !BIT_SET(buddy->flags, PAGE_FLAG_USED)) {
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
        primary_page->order++;

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
  return (void *) ((get_index(page) * PAGE_SIZE) + mem_start);
}

static void paging_init() {
    uint32_t page_table_start = mem_start;
    //reserve space for the page tables, remeaining page aligned, page directory is declared in the BSS above
    mem_start += DIV_UP(8 * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE;

    //reserve space for the page_t structs, remaining page aligned
    pages = (page_t *) mem_start;
    num_pages = DIV_DOWN(mem_end - mem_start, PAGE_SIZE);
    mem_start += DIV_UP(sizeof(page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE;

    uint32_t total = mem_end - page_table_start;
    uint32_t paging_overhead = DIV_UP(8 * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE;
    uint32_t malloc_overhead = DIV_UP(sizeof(page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE;
    uint32_t available = mem_end - mem_start;
    kprintf("Total     RAM:   %4u MB (%7u pages)\n", DIV_DOWN(total,           1024 * 1024), DIV_DOWN(total,           PAGE_SIZE));
    kprintf("Available RAM:   %4u MB (%7u pages)\n", DIV_DOWN(available,       1024 * 1024), DIV_DOWN(available,       PAGE_SIZE));
    kprintf("Paging Overhead: %4u MB (%7u pages)\n", DIV_DOWN(paging_overhead, 1024 * 1024), DIV_DOWN(paging_overhead, PAGE_SIZE));
    kprintf("MAlloc Overhead: %4u MB (%7u pages)\n", DIV_DOWN(malloc_overhead, 1024 * 1024), DIV_DOWN(malloc_overhead, PAGE_SIZE));
    kprintf("Physical Address Space: 0x%08X - 0x%08X\n", page_table_start, mem_end);

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

    for(uint32_t i = 0; i < NUM_ENTRIES; i++) {
        uint32_t page_mem_start_addr = i * PAGE_SIZE * 1024;

        uint32_t *page_table = (uint32_t *) page_table_start; //page_table_start is already page aligned
        page_table_start += PAGE_SIZE;

        page_directory[i] = (uint32_t) page_table | 1 /* present */ | 2 /* read/write */;
        for(uint32_t j = 0; j < NUM_ENTRIES; j++) {
             page_table[j] = (page_mem_start_addr + j * PAGE_SIZE) | 1 /* present */;
             if(!(i == 0 && j == 0) && (page_mem_start_addr < 0x00120000 /* start of code */ || page_mem_start_addr >= (uint32_t) &end_of_image)) {
                 page_table[j] |= 2;
             }
        }
    }

    __asm__ volatile("mov %0, %%cr3":: "b" (page_directory));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=b" (cr0));
    cr0 |= 1 << 31; //enable paging
    cr0 |= 1 << 16; //enable read-only protection in supervisor mode
    __asm__ volatile("mov %0, %%cr0":: "b" (cr0));
}

void mm_init(multiboot_info_t *mbd) {
    kernel_end = (uint32_t) &end_of_image;

    for(uint32_t i = 0; i < module_count(); i++) {
        uint32_t end = module_get(i)->end - 1;
        if(kernel_end < end) {
            kernel_end = end;
        }
    }

    kprintf("Kernel image ends at 0x%08X\n", kernel_end);

    multiboot_memory_map_t *mmap = mbd->mmap;
    uint64_t end_addr;
    uint32_t start_addr = ((kernel_end + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE, best_len = 0, idx = -1;
    for(uint32_t i = 0; i < (mbd->mmap_length / sizeof(multiboot_memory_map_t)); i++) {
        if(mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
             if(mmap[i].addr >= ADDRESS_SPACE_SIZE) {
                 continue;
             }
             end_addr = mmap[i].addr + mmap[i].len;
             if(end_addr > ADDRESS_SPACE_SIZE) {
                 end_addr = ADDRESS_SPACE_SIZE - 1;
             }

             if(mmap[i].addr > start_addr) {
                 start_addr = (uint32_t) mmap[i].addr;
             }

             uint32_t aligned_start = DIV_UP(start_addr, PAGE_SIZE) * PAGE_SIZE, aligned_end = DIV_DOWN(end_addr, PAGE_SIZE) * PAGE_SIZE, len = aligned_end - aligned_start;
             if(aligned_start > aligned_end || len < best_len) {
                 continue;
             }
             idx = i;
             best_len = len;

             //page align start_addr and end_addr
             mem_start = aligned_start;
             mem_end = aligned_end;
        }
    }

    for(uint32_t i = 0; i < (mbd->mmap_length / sizeof(multiboot_memory_map_t)); i++) {
        if(idx == i) {
            console_color(0x0A);
        } else if(mmap[i].type != MULTIBOOT_MEMORY_AVAILABLE) {
            console_color(0x0C);
        } else {
            console_color(0x0E);
        }

        kprintf("    - %4u MB (0x%08X - 0x%08X)\n",
        ((uint32_t) mmap[i].len) / (1024 * 1024),
        ((uint32_t) mmap[i].addr),
        ((uint32_t) mmap[i].addr) + ((uint32_t) MIN(mmap[i].len, ADDRESS_SPACE_SIZE - mmap[i].addr - 1)));
    }
    console_color(0x07);

    if(idx == ((uint32_t) -1)) panic("MM - did not find suitable memory region");

    paging_init();
}

