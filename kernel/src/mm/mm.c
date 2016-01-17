#include <stddef.h>
#include <stdbool.h>

#include "lib/int.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/math.h"
#include "common/compiler.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "sched/task.h"
#include "fs/module.h"
#include "log/log.h"
#include "misc/stats.h"

#define MMAP_BUFF_SIZE 256

#define MAX_ORDER 10
#define ADDRESS_SPACE_SIZE 4294967295ULL

#define NUM_ENTRIES 1024

#define KERNEL_TABLES (NUM_ENTRIES / 4)

#define PAGE_FLAG_USED (1 << 0)
#define PAGE_FLAG_PERM (1 << 1)
#define PAGE_FLAG_USER (1 << 2)

//TODO asynchronously free the boot mem data (as a task after all CPUs have come up)

extern uint32_t image_start;
extern uint32_t image_end;

extern uint32_t init_mem_start;
extern uint32_t init_mem_end;

static uint32_t kernel_start;
static uint32_t kernel_end;
static uint32_t malloc_start;

__initdata uint32_t mods_count;

uint32_t page_directory[1024] ALIGN(PAGE_SIZE);
static uint32_t kernel_page_tables[255][1024] ALIGN(PAGE_SIZE);
static uint32_t kernel_next_page; //page which will next be mapped to kernel addr space on alloc
static page_t *pages;
static page_t *free_page_list[MAX_ORDER + 1];
static __initdata uint8_t mmap_buff[MMAP_BUFF_SIZE];

static DEFINE_SPINLOCK(alloc_lock);
static DEFINE_SPINLOCK(map_lock);

static inline void invlpg(uint32_t addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static uint32_t get_index(page_t *page) {
    return (((uint32_t) page) - ((uint32_t) pages)) / (sizeof (page_t));
}

static page_t * get_buddy(page_t *page) {
    return page + ((1 << page->order) * ((DIV_DOWN(get_index(page), (1 << page->order)) % 2) ? -1 : 1));
}

static inline void flag_set(page_t *page, int8_t flag) {
    page->flags |= flag;
}

static inline void flag_unset(page_t *page, int8_t flag) {
    page->flags &= ~flag;
}

void * page_to_phys(page_t *page) {
    return (void *) (get_index(page) * PAGE_SIZE);
}

void * page_to_virt(page_t *page) {
    return (void *) (page->addr);
}

page_t * phys_to_page(void *addr) {
    return &pages[((uint32_t) addr) / PAGE_SIZE];
}

void * virt_to_phys(void *addr) {
    uint32_t num = ((((uint32_t) addr) & 0xFFFFF000) - VIRTUAL_BASE) / PAGE_SIZE;

    return (void *) ((kernel_page_tables[num / NUM_ENTRIES][num % NUM_ENTRIES] & 0xFFFFF000) | (((uint32_t) addr) & 0xFFF));
}

static inline void add_to_freelist(page_t *page) {
    page->next = free_page_list[page->order];
    if(free_page_list[page->order]) free_page_list[page->order]->prev = page;
    free_page_list[page->order] = page;
}

static inline page_t * split_block(page_t *master) {
    uint32_t order = master->order;
    BUG_ON(!order);

    master->order--;

    page_t *buddy = get_buddy(master);
    BUG_ON(!buddy);

    buddy->order = master->order;

    return buddy;
}

static inline page_t * alloc_block_exact(uint32_t order, uint32_t actual) {
    page_t *block = free_page_list[order];
    if(block->next) block->next->prev = NULL;
    free_page_list[order] = block->next;

    page_t *current = block;
    page_t *buddy;

    while(actual) {
        buddy = split_block(current);
        order--;

        BUG_ON(order != buddy->order);

        if(actual <= (uint32_t) (1 << order)) {
            add_to_freelist(buddy);
        } else {
            current = buddy;
        }

        if(actual >= (uint32_t) (1 << order)) {
            actual -= 1 << order;
        }
    }

    return block;
}

static page_t * do_alloc_pages(uint32_t number, uint32_t UNUSED(flags)) {
    uint32_t target_order = fls32(number);
    if(number ^ (1 << target_order)) {
        target_order++;
    }

    for (uint32_t order = target_order; order < MAX_ORDER + 1; order++) {
        if (free_page_list[order] != NULL) {
            page_t *alloced = alloc_block_exact(order, number);

            for(uint32_t i = 0; i < number; i++) {
                flag_set(&alloced[i], PAGE_FLAG_USED);
            }

            pages_in_use += number;

            return alloced;
        }
    }

    panic("OOM!");
}

#define PRESENT     (1 << 0)
#define WRITABLE    (1 << 1)
#define USER        (1 << 2)

static void * do_map_page(const void *phys) {
    kernel_page_tables[kernel_next_page / 1024][kernel_next_page % 1024] = (((uint32_t) phys) & 0xFFFFF000) | WRITABLE | PRESENT;
    return (void *) ((kernel_next_page++ * PAGE_SIZE) + (((uint32_t) phys) & 0xFFF) + VIRTUAL_BASE);
}

void * map_page(const void *phys) {
    return map_pages(phys, 1);
}

void * map_pages(const void *phys, uint32_t pages) {
    uint32_t flags;
    spin_lock_irqsave(&map_lock, &flags);

    void *virt = do_map_page(phys);
    for(uint32_t i = 1; i < pages; i++) {
        do_map_page((void *) (((uint32_t) phys) + (PAGE_SIZE * i)));
    }

    spin_unlock_irqstore(&map_lock, flags);

    return virt;
}

void page_build_directory(uint32_t directory[]) {
    for (uint32_t i = 0; i < NUM_ENTRIES - KERNEL_TABLES; i++) {
        directory[i] = 0;
    }

    for (uint32_t i = 0; i < KERNEL_TABLES; i++) {
        directory[i + (VIRTUAL_BASE / PAGE_SIZE / NUM_ENTRIES)] = (((uint32_t) &kernel_page_tables[i]) - VIRTUAL_BASE) | PRESENT | WRITABLE;
    }
}

void * alloc_page_user(uint32_t flags, task_t *task, uint32_t addr) {
    page_t *page = alloc_page(flags);

    flag_set(page, PAGE_FLAG_USER);

    if(task->directory[addr / (PAGE_SIZE * 1024)] == 0) {
        page_t *table = alloc_page(0);
        task_add_page(task, table);

        memset(page_to_virt(table), 0, PAGE_SIZE);

        task->directory[addr / (PAGE_SIZE * 1024)] = ((uint32_t) page_to_phys(table)) | PRESENT | WRITABLE | USER;
    }

    ((uint32_t *) page_to_virt(phys_to_page((void *) (task->directory[addr / (PAGE_SIZE * 1024)] & ~0xFFF))))[(addr % (PAGE_SIZE * 1024)) / PAGE_SIZE] = ((uint32_t) page_to_phys(page)) | WRITABLE | PRESENT | USER;

    invlpg(addr);

    task_add_page(task, page);
    return page_to_virt(page);
}

page_t * alloc_page(uint32_t flags) {
    return alloc_pages(1, flags);
}

page_t * alloc_pages(uint32_t num, uint32_t flags) {
    uint32_t f;
    spin_lock_irqsave(&alloc_lock, &f);

    page_t *pages = do_alloc_pages(num, flags);
    void *first = map_pages(page_to_phys(pages), num);
    for(uint32_t i = 0; i < num; i++) {
        pages[i].addr = ((uint32_t) first) + (PAGE_SIZE * i);
    }

    spin_unlock_irqstore(&alloc_lock, f);

    return pages;
}

void free_page(page_t *page) {
    uint32_t f;
    spin_lock_irqsave(&alloc_lock, &f);

    BUG_ON(BIT_SET(page->flags, PAGE_FLAG_PERM));
    BUG_ON(!BIT_SET(page->flags, PAGE_FLAG_USED));

    flag_unset(page, PAGE_FLAG_USED);
    flag_unset(page, PAGE_FLAG_USER);

    page_t *buddy = get_buddy(page);
    while (buddy != NULL && !BIT_SET(buddy->flags, PAGE_FLAG_USED) && buddy->order == page->order && page->order < MAX_ORDER) {
        if (buddy->prev) {
            buddy->prev->next = buddy->next;
        } else {
            free_page_list[buddy->order] = buddy->next;
        }

        if (buddy->next) {
            buddy->next->prev = buddy->prev;
        }

        uint32_t page_index = get_index(page);
        pages[page_index - (page_index % 2)].order = page->order + 1;
        page = &pages[page_index - (page_index % 2)];

        buddy = get_buddy(page);
    }

    if (free_page_list[page->order]) {
        free_page_list[page->order]->prev = page;
    }

    page->prev = NULL;
    page->next = free_page_list[page->order];

    free_page_list[page->order] = page;

    pages_in_use--;

    spin_unlock_irqstore(&alloc_lock, f);
}

void free_pages(page_t *pages, uint32_t count) {
    for(uint32_t i = 0; i < count; i++) {
        free_page(&pages[i]);
    }
}

static void claim_page(uint32_t idx) {
    flag_unset(&pages[idx], PAGE_FLAG_PERM);
    free_page(&pages[idx]);
    pages_avaliable++;
}

void __init mm_init() {
    multiboot_module_t *mods = multiboot_info->mods;
    mods_count = multiboot_info->mods_count;

    kernel_start = ((uint32_t) &image_start);
    kernel_end = ((uint32_t) &image_end);

    for (uint32_t i = 0; i < mods_count; i++) {
        uint32_t start = mods[i].start;
        uint32_t end = mods[i].end;

        if (kernel_start > start) {
            kernel_start = start;
        }

        if (kernel_end < end) {
            kernel_end = end;
        }
    }

    //preserve the mmap_length struct
    uint32_t mmap_length = multiboot_info->mmap_length;
    memcpy(mmap_buff, multiboot_info->mmap, mmap_length);
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *) mmap_buff;

    kprintf("mm - kernel image 0x%p-0x%p", kernel_start, kernel_end);

    uint32_t best_len = 0;
    for (uint32_t i = 0; i < mmap_length / sizeof (multiboot_memory_map_t); i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmap[i].addr >= ADDRESS_SPACE_SIZE) {
                continue;
            }

            uint32_t start = mmap[i].addr;
            uint64_t end = mmap[i].addr + mmap[i].len;

            if (end > ADDRESS_SPACE_SIZE) {
                end = ADDRESS_SPACE_SIZE;
            }

            if (start <= kernel_start && kernel_start < end) {
                if (kernel_end < end) {
                    if (kernel_start - start > end - kernel_end) {
                        end = kernel_start;
                    } else {
                        start = kernel_end;
                    }
                } else {
                    end = kernel_start;
                }
            } else if (start <= kernel_end && kernel_end < end) {
                start = kernel_end;
            } else if (kernel_start <= start && end <= kernel_end) {
                continue;
            }

            start = DIV_UP(start, PAGE_SIZE) * PAGE_SIZE;
            end = DIV_DOWN(end, PAGE_SIZE) * PAGE_SIZE;

            if (start >= end) {
                continue;
            }

            if (end - start > best_len) {
                best_len = end - start;
                malloc_start = start;
            }
        }
    }

    if (best_len < DIV_UP(sizeof (uint32_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE) panic("MM - could not find a sufficiently large contiguous memory region");

    pages = (page_t *) (malloc_start + VIRTUAL_BASE);

    //map kernel sections
    uint32_t kernel_num_pages = DIV_UP(kernel_end, PAGE_SIZE);
    for (uint32_t page = 0; page < kernel_num_pages; page++) {
        kernel_page_tables[page / 1024][page % 1024] = (page * PAGE_SIZE) | WRITABLE | PRESENT;
    }

    //map the massive page_t struct array
    uint32_t malloc_num_pages = DIV_UP(sizeof (page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE);
    for (uint32_t page = kernel_num_pages; page <= kernel_num_pages + malloc_num_pages; page++) {
        kernel_page_tables[page / 1024][page % 1024] = (malloc_start + ((page - kernel_num_pages) * PAGE_SIZE)) | WRITABLE | PRESENT;
    }

    kernel_next_page = kernel_num_pages + malloc_num_pages + 1;

    page_build_directory(page_directory);

    __asm__ volatile("mov %0, %%cr3" :: "a" (((uint32_t) page_directory) - VIRTUAL_BASE));

    debug_map_virtual();

    multiboot_info = (multiboot_info_t *) map_page(multiboot_info);
    if(mods)
        mods = multiboot_info->mods = (multiboot_module_t *) map_page(mods);

    for (uint32_t i = 0; i < mods_count; i++) {
        uint32_t start = mods[i].start;
        uint32_t end = mods[i].end;
        uint32_t len = end - start;
        uint32_t num_pages = DIV_UP(end - start, PAGE_SIZE);

        mods[i].start = (uint32_t) map_pages((void*) start, num_pages);
        mods[i].end = mods[i].start + len;
    }

    //this may arbitrarily corrupt multiboot data structures, including the mmap structure, which caused a bug
    for (uint32_t page = 0; page < NUM_ENTRIES * NUM_ENTRIES; page++) {
        pages[page].flags = PAGE_FLAG_PERM | PAGE_FLAG_USED;
        pages[page].order = 0;
        pages[page].next = NULL;
        pages[page].prev = NULL;
    }

    uint32_t malloc_page_end = DIV_DOWN(malloc_start, PAGE_SIZE) + malloc_num_pages;

    for (uint32_t i = 0; i < mmap_length / sizeof(multiboot_memory_map_t); i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {

            if (mmap[i].addr >= ADDRESS_SPACE_SIZE) {
                continue;
            }

            uint32_t start = mmap[i].addr;
            uint64_t end = mmap[i].addr + mmap[i].len;

            if (end > ADDRESS_SPACE_SIZE) {
                end = ADDRESS_SPACE_SIZE;
            }

            start = start == 0 ? 0 : DIV_UP(start, PAGE_SIZE);
            end = end == 0 ? 0 : DIV_DOWN(end, PAGE_SIZE);

            for (uint32_t j = end; j > start; j--) {
                if (j >= malloc_page_end) {
                    claim_page(j);
                }
            }

            pages_in_use = 0;
        }
    }

    kprintf("mm - paging: %u MB, malloc: %u MB, avaliable: %u MB",
            DIV_DOWN(DIV_UP(sizeof (uint32_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
            DIV_DOWN(DIV_UP(sizeof (page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
            DIV_DOWN(pages_avaliable * PAGE_SIZE, 1024 * 1024));
}

void mm_postinit_reclaim() {
    uint32_t pages = DIV_UP(((uint32_t) &init_mem_end) - ((uint32_t) &init_mem_start), PAGE_SIZE);

    kprintf("mm - reclaiming %u init pages (0x%X-0x%X)", pages, &init_mem_start, &init_mem_end);

    for(uint32_t i = 0; i < pages; i++) {
        claim_page(DIV_DOWN(((uint32_t) &init_mem_start), PAGE_SIZE) + i);
    }
}
