#include <stddef.h>
#include <stdbool.h>

#include "int.h"
#include "string.h"
#include "init.h"
#include "common.h"
#include "panic.h"
#include "mm.h"
#include "cache.h"
#include "module.h"
#include "console.h"
#include "debug.h"
#include "task.h"
#include "log.h"

#define MAX_ORDER 10
#define ADDRESS_SPACE_SIZE 4294967296ULL

#define NUM_ENTRIES 1024

#define KERNEL_TABLES (NUM_ENTRIES / 4)

#define PAGE_FLAG_USED (1 << 0)
#define PAGE_FLAG_PERM (1 << 1)
#define PAGE_FLAG_USER (1 << 2)

extern uint32_t image_start;
extern uint32_t image_end;

static uint32_t kernel_start;
static uint32_t kernel_end;
static uint32_t malloc_start;

static uint32_t page_directory[1024] ALIGN(PAGE_SIZE);
static uint32_t kernel_page_tables[255][1024] ALIGN(PAGE_SIZE);
static uint32_t kernel_next_page; //page which will next be mapped to kernel addr space on alloc
static page_t *pages;
static page_t *free_page_list[MAX_ORDER + 1];

static inline void invlpg(uint32_t addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static uint32_t get_index(page_t * page) {
    return (((uint32_t) page) - ((uint32_t) pages)) / (sizeof (page_t));
}

static page_t * get_buddy(page_t * page) {
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

static page_t * do_alloc_page(uint32_t UNUSED(flags)) {
    for (uint32_t order = 0; order < MAX_ORDER + 1; order++) {
        if (free_page_list[order] != NULL) {
            for (; order > 0; order--) {
                page_t * page = free_page_list[order];
                page->order--;

                page_t * buddy = get_buddy(page);

                free_page_list[order] = page->next;

                page->prev = NULL;

                if (buddy == NULL) {
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
            flag_set(alloced, PAGE_FLAG_USED);

            if (free_page_list[0]->next) {
                free_page_list[0]->next->prev = NULL;
            }

            free_page_list[0] = free_page_list[0]->next;

            return alloced;
        }
    }

    panic("OOM!");
}

#define PRESENT     (1 << 0)
#define WRITABLE    (1 << 1)
#define USER        (1 << 2)

void * mm_map(const void *phys) {
    kernel_page_tables[kernel_next_page / 1024][kernel_next_page % 1024] = (((uint32_t) phys) & 0xFFFFF000) | WRITABLE | PRESENT;

    return (void *) ((kernel_next_page++ * PAGE_SIZE) + (((uint32_t) phys) & 0xFFF) + VIRTUAL_BASE);
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
    page_t *page = do_alloc_page(flags);
    page->addr = (uint32_t) mm_map(page_to_phys(page));

    return page;
}

void free_page(page_t *page) {
    BUG_ON(BIT_SET(page->flags, PAGE_FLAG_PERM));
    BUG_ON(!BIT_SET(page->flags, PAGE_FLAG_USED));

    flag_unset(page, PAGE_FLAG_USED);
    flag_unset(page, PAGE_FLAG_USER);

    page_t *buddy = get_buddy(page);
    while (buddy != NULL && !BIT_SET(buddy->flags, PAGE_FLAG_USED) && buddy->order == page->order) {
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
}

static INITCALL mm_init() {
    kernel_start = ((uint32_t) &image_start);
    kernel_end = ((uint32_t) &image_end);

    for (uint32_t i = 0; i < multiboot_info->mods_count; i++) {
        uint32_t end = multiboot_info->mods[i].end - 1;
        if (kernel_end < end) {
            kernel_end = end;
        }
    }

    logf("mm - kernel image 0x%p-0x%p", kernel_start, kernel_end);

    multiboot_memory_map_t *mmap = multiboot_info->mmap;
    uint32_t best_len = 0;
    for (uint32_t i = 0; i < (multiboot_info->mmap_length / sizeof (multiboot_memory_map_t)); i++) {
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmap[i].addr >= ADDRESS_SPACE_SIZE) {
                continue;
            }

            uint32_t start = mmap[i].addr;
            uint64_t end = mmap[i].addr + mmap[i].len;

            if (end > ADDRESS_SPACE_SIZE) {
                end = ADDRESS_SPACE_SIZE - 1ULL;
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

    if (best_len < DIV_UP(sizeof (uint32_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE) panic("MM - could not find sufficiently large contiguous memory region");

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

    __asm__ volatile("mov %0, %%cr3"::"a" (((uint32_t) page_directory) - VIRTUAL_BASE));

    console_map_virtual();
    debug_map_virtual();
    multiboot_info = (multiboot_info_t *) mm_map(multiboot_info);
    multiboot_info->mods = (multiboot_module_t *) mm_map(multiboot_info->mods);
    mmap = (multiboot_memory_map_t *) mm_map(mmap);

    for (uint32_t page = 0; page < NUM_ENTRIES * NUM_ENTRIES; page++) {
        pages[page].flags = PAGE_FLAG_PERM | PAGE_FLAG_USED;
        pages[page].order = 0;
        pages[page].next = NULL;
        pages[page].prev = NULL;
    }

    uint32_t available_pages = 0;
    uint32_t malloc_page_end = DIV_UP(malloc_start, PAGE_SIZE) + malloc_num_pages;

    for (uint32_t i = (multiboot_info->mmap_length / sizeof(multiboot_memory_map_t)); i > 0; i--) {
        if (mmap[i - 1].type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmap[i - 1].addr >= ADDRESS_SPACE_SIZE) {
                continue;
            }

            uint32_t start = mmap[i - 1].addr;
            uint64_t end = mmap[i - 1].addr + mmap[i - 1].len;

            if (end > ADDRESS_SPACE_SIZE) {
                end = ADDRESS_SPACE_SIZE - 1ULL;
            }

            start = start == 0 ? 0 : DIV_UP(start, PAGE_SIZE);
            end = end == 0 ? 0 : DIV_DOWN(end, PAGE_SIZE);

            for (uint32_t j = end; j > start; j--) {
                if (j - 1 >= malloc_page_end) {
                    flag_unset(&pages[j - 1], PAGE_FLAG_PERM);
                    free_page(&pages[j - 1]);
                    available_pages++;
                }
            }
        }
    }

    //TODO mm_unmap(mmap);

    logf("mm - paging: %u MB, malloc: %u MB, avaliable: %u MB",
            DIV_DOWN(DIV_UP(sizeof (uint32_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
            DIV_DOWN(DIV_UP(sizeof (page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
            DIV_DOWN(available_pages * PAGE_SIZE, 1024 * 1024));

    return cache_init();
}

core_initcall(mm_init);
