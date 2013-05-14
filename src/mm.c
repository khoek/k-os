#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "mm.h"
#include "common.h"
#include "panic.h"
#include "console.h"

#define ADDRESS_SPACE_SIZE 4294967296ULL

#define MAX_ORDER 10

#define DIV_DOWN(x, y) ((x) / (y))
#define DIV_UP(x, y) ((((x) - 1) / (y)) + 1)

#define PAGE_FLAG_USED    0x01

#define BIT_SET(bits, bit) (((uint32_t) bits) & ((uint32_t) bit))

extern uint32_t end_of_image;
static uint32_t kernel_end;

static uint32_t num_pages;
static uint32_t mem_start;
static uint32_t mem_end;
//static uint32_t current_increment;

static uint32_t page_directory[1024] __attribute__((aligned(PAGE_SIZE)));
static page_t * pages;
static page_t * free_page_list[MAX_ORDER + 1];

static void paging_init(uint32_t max_addr) {
    // this maps in page table entries up to
    uint32_t max_dir_ent = max_addr / (PAGE_SIZE * 1024);
    uint32_t page_table_space = mem_start;
    mem_start += PAGE_SIZE * (max_dir_ent + 1);

    pages = (page_t *) mem_start;

    uint32_t len = mem_end - mem_start;
    uint32_t num_pages = DIV_DOWN(len, PAGE_SIZE);

    //truncate len to a multiple of PAGE_SIZE and update end
    len = num_pages * PAGE_SIZE;
    mem_end = ((uint32_t) pages) + len;

    //reserve space for the page_t structs, and keep mem_start page aligned
    mem_start += DIV_UP(sizeof(page_t) * num_pages, PAGE_SIZE) * PAGE_SIZE;

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

    for(uint32_t i = 0; i <= max_dir_ent; i++) {
        //calculate page mem_start address:
        uint32_t page_mem_start_addr = i * PAGE_SIZE * 1024;

        //reserve 4 KiB for page table, mem_start is already page aligned...
        uint32_t *page_table = (uint32_t *) page_table_space;
        page_table_space += PAGE_SIZE;

        //construct page table:
        page_directory[i] = (uint32_t) page_table | 1 /* present */ | 2 /* read/write */;
        for(uint32_t j = 0; j < 1024; j++) {
             page_table[j] = (page_mem_start_addr + j * PAGE_SIZE) | 1 /* present */;
             if(!(i == 0 && j == 0) && (page_mem_start_addr < 0x00120000 /* start of code */ || page_mem_start_addr >= (uint32_t) &end_of_image)) {
                 page_table[j] |= 2; // allow writes to non-code pages
             }
        }
    }
    
    if(1) return; // FIXME, add working paging! :D
    
    __asm__ volatile("mov %0, %%cr3":: "b" (page_directory));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0": "=b" (cr0));
    cr0 |= 1 << 31; //enable paging
    cr0 |= 1 << 16; //enforce read-only page protection (through GPFs)    
    __asm__ volatile("mov %0, %%cr0":: "b" (cr0));
}

static uint32_t get_index(page_t * page) {
    return (((uint32_t) page) - ((uint32_t) pages)) / (sizeof(page_t));
}

static page_t * get_buddy(page_t * page) {
    uint32_t index = get_index(page);

    if(index > (num_pages - 1)) {
        return NULL;
    }

    if((DIV_DOWN(index, (1 << page->order)) % 2)) {
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

void mm_init(multiboot_info_t *mbd) {    
    kernel_end = (uint32_t) &end_of_image;

    multiboot_module_t *mods = mbd->mods;
    for(uint32_t i = 0; i < mbd->mods_count; i++) {
        if(mods[i].mod_end > kernel_end) {
            kernel_end = mods[i].mod_end;
        }
    }
    
    kprintf("Kernel image ends at 0x%08X\n", kernel_end);

    bool found = false;
    multiboot_memory_map_t *mmap = mbd->mmap;
    uint32_t start_addr = ((kernel_end + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE, best_len = 0;
    uint64_t end_addr;
    for(uint32_t i = 0; i < (mbd->mmap_length / sizeof(multiboot_memory_map_t)); i++) {
        kprintf("    - %s %4u MB (0x%08X - 0x%08X)\n",
        (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) ? " " : "R",
        ((uint32_t) mmap[i].len) / (1024 * 1024),
        ((uint32_t) mmap[i].addr),
        ((uint32_t) mmap[i].addr) + ((uint32_t) mmap[i].len));
    
        if(mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
             if(mmap[i].addr >= ADDRESS_SPACE_SIZE) {
                 continue;
             }
             end_addr = mmap[i].addr + mmap[i].len;
             if(end_addr > ADDRESS_SPACE_SIZE) {
                 end_addr = ADDRESS_SPACE_SIZE - 1;
             }
             
             uint32_t len = end_addr - mmap[i].addr;
             if(len < best_len || mmap[i].addr + len <= start_addr) {
                 continue;
             }             
             best_len = len;
             
             if(mmap[i].addr > start_addr) {
                 start_addr = (uint32_t) mmap[i].addr;
             }
             // page align mem_start address:
             mem_start = ((start_addr + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
             mem_end = end_addr;
             found = true;
        }
    }
    
    if(!found) panic("MM - did not find suitable memory region");
    
    kprintf("%u MB available memory (0x%08X - 0x%08X)\n", (mem_end - mem_start) / (1024 * 1024), mem_start, mem_end);
    paging_init(mem_end);
}

//void * sbrk(int32_t increment) {
//    if(current_increment + increment >= max_size) return NULL;
//    if(increment < 0 && ((uint32_t) -increment) > current_increment) panic("MM - trying to sbrk to negative current_increment");
//
//    void* ptr = (void *)(mem_start + current_increment);
//    current_increment += increment;
//
//    return ptr;
//}
