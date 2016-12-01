#include <stdbool.h>

#include "common/types.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "init/multiboot.h"
#include "common/math.h"
#include "common/compiler.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "sched/task.h"
#include "log/log.h"
#include "misc/stats.h"

#define MMAP_BUFF_SIZE 256

#define MAX_ORDER 10
#define ADDRESS_SPACE_SIZE 4294967295ULL

#define PAGE_TABLE_EXTENT (PAGE_SIZE * NUM_ENTRIES)

#define PAGE_FLAG_USED  (1 << 0)
#define PAGE_FLAG_PERM  (1 << 1)
#define PAGE_FLAG_USER  (1 << 2)
#define PAGE_FLAG_CACHE (1 << 3)

#define IS_CACHE_PAGE(page) (page->flags & PAGE_FLAG)

//TODO asynchronously free the boot stack, etc. (as a task after all CPUs have come up)

extern uint32_t image_start;
extern uint32_t image_end;

extern uint32_t init_mem_start;
extern uint32_t init_mem_end;

static phys_addr_t kernel_start;
static phys_addr_t kernel_end;
static phys_addr_t malloc_start;

__initdata uint32_t mods_count;

page_t *pages;
static list_head_t free_page_list[MAX_ORDER + 1];
static __initdata uint8_t mmap_buff[MMAP_BUFF_SIZE];
static uint32_t mmap_length;

static DEFINE_SPINLOCK(alloc_lock);

static inline uint32_t get_order_idx(uint32_t idx, uint32_t order) {
    return DIV_DOWN(idx, 1ULL << order);
}

static inline bool is_free(page_t *page) {
    return !(page->flags & PAGE_FLAG_USED) && !(page->flags & PAGE_FLAG_PERM);
}

static inline bool is_master(page_t *page) {
    return !(get_order_idx(get_index(page), page->order) % 2);
}

static inline bool is_subblock_master(page_t *page) {
    if(!page->order) return true;
    return !(get_order_idx(get_index(page), page->order - 1) % 2);
}

static inline page_t * get_buddy(page_t *page) {
    if(is_master(page)) {
        return page + (1ULL << page->order);
    } else {
        return page - (1ULL << page->order);
    }
}

static PURE inline page_t * get_master(page_t *page) {
    if(is_master(page)) {
        return page;
    } else {
        return get_buddy(page);
    }
}

#ifdef CONFIG_DEBUG_MM
static inline void check_not_in_freelists(page_t *page) {
    page_t *ent;
    LIST_FOR_EACH_ENTRY(ent, &free_page_list[page->order], list) {
        if(ent == page) {
            BUG();
        }
    }
}
#else
static inline void check_not_in_freelists(page_t *page) {
}
#endif

static void add_to_freelists(page_t *page) {
    BUG_ON(!is_free(page));
    BUG_ON(!is_subblock_master(page));
    check_not_in_freelists(page);
    list_add(&page->list, &free_page_list[page->order]);
}

//Cuts "master" in half. Returns the orphaned half of "master".
static inline page_t * split_block(page_t *master) {
    BUG_ON(!is_subblock_master(master));
    BUG_ON(!master->order);

    master->order--;

    page_t *slave = get_buddy(master);

    BUG_ON(master->order != slave->order);
    BUG_ON(!is_free(master));
    BUG_ON(!is_free(slave));

    return slave;
}

//Joins page with its buddy, irrespective of which is the master/slave. Returns
//the master.
static inline page_t * join_block(page_t *page) {
    page_t *master = get_master(page);
    page_t *buddy = get_buddy(master);

    BUG_ON(master->order != get_buddy(master)->order);
    BUG_ON(master->order == MAX_ORDER);

    BUG_ON(!is_free(master));
    BUG_ON(!is_free(buddy));

    check_not_in_freelists(master);
    check_not_in_freelists(buddy);

    master->order++;

    return master;
}

//Chop off unused pages at the end of a block.
static inline void trim_block(page_t *block, uint32_t actual) {
    BUG_ON(!actual);
    BUG_ON(!is_free(block));
    check_not_in_freelists(block);

    page_t *buddy;
    while(actual) {
        uint32_t block_size = 1ULL << block->order;

        //Do we need to subdivide?
        if(actual == block_size) {
            return;
        }

        //We can't subdivide a single page.
        BUG_ON(!block->order);
        //The block should not be in use already.
        BUG_ON(!is_free(block));
        //The block must be at least as big as actual.
        BUG_ON(block_size < actual);

        //We are going to have to split the block.
        buddy = split_block(block);

        //If we don't need the buddy any more, release it to the freelists.
        if(actual <= block_size >> 1) {
            BUG_ON(!is_free(buddy));
            add_to_freelists(buddy);
            continue;
        }

        //We have allocated a piece of the requested block and now we need to
        //subdivide the buddy to obtain the rest of the requested space.
        actual ^= (1ULL << block->order);
        block = buddy;
    }

    BUG();
}

//Start with the given block. Continue amalgamating the largest free block which
//contains the original block with its buddy (if free) until this is no longer
//possible. Then mark the big block as free.
static inline void ripple_join(page_t *page) {
    BUG_ON(!is_free(page));

    page_t *block = page;
    page_t *buddy = get_buddy(block);
    while(is_free(buddy)
        && buddy->order == block->order
        && block->order < MAX_ORDER) {
        list_rm(&buddy->list);

        check_not_in_freelists(block);
        check_not_in_freelists(buddy);

        block = join_block(block);
        buddy = get_buddy(block);

        BUG_ON(!is_free(block));
    }

    add_to_freelists(block);
}

static page_t * do_alloc_pages(uint32_t number) {
    uint32_t target_order = log2(number);
    if(number ^ (1 << target_order)) {
        target_order++;
    }

    for(uint32_t order = target_order; order <= MAX_ORDER; order++) {
        if(!list_empty(&free_page_list[order])) {
            page_t *block = list_first(&free_page_list[order], page_t, list);
            BUG_ON(block->flags & PAGE_FLAG_USED);
            list_rm(&block->list);
            trim_block(block, number);

            for(uint32_t i = 0; i < number; i++) {
                BUG_ON(block[i].flags);
                block[i].flags |= PAGE_FLAG_USED;
            }

            pages_in_use += number;

            return block;
        }
    }

    panicf("OOM! wanted %X (%X/%X)", number, pages_in_use, pages_avaliable);
}

page_t * alloc_page(uint32_t flags) {
    return alloc_pages(1, flags);
}

page_t * alloc_pages(uint32_t num, uint32_t flags) {
    uint32_t f;
    spin_lock_irqsave(&alloc_lock, &f);

    page_t *pages = do_alloc_pages(num);
    void *first = map_pages(page_to_phys(pages), num);
    for(uint32_t i = 0; i < num; i++) {
        pages[i].addr = ((uint32_t) first) + (PAGE_SIZE * i);

        BUG_ON(!(pages[i].flags & PAGE_FLAG_USED));
        BUG_ON(pages[i].flags & PAGE_FLAG_PERM);
        check_not_in_freelists(&pages[i]);

        if(flags & ALLOC_CACHE) {
            pages[i].flags |= PAGE_FLAG_CACHE;
        }
    }

    if(flags & ALLOC_COMPOUND) {
        pages->compound_num = num;
    }

    if(flags & ALLOC_ZERO) {
        memset(first, 0, num * PAGE_SIZE);
    }

    spin_unlock_irqstore(&alloc_lock, f);

    return pages;
}

void free_page(page_t *page) {
    uint32_t f;
    spin_lock_irqsave(&alloc_lock, &f);

    uint32_t page_size =  1 << page->order;

    pages_in_use -= page_size;

    for(uint32_t i = 0; i < page_size; i++) {
        BUG_ON(page[i].flags & PAGE_FLAG_PERM);
        BUG_ON(!(page[i].flags & PAGE_FLAG_USED));

        page[i].flags = 0;
    }

    ripple_join(page);

    spin_unlock_irqstore(&alloc_lock, f);
}

void free_pages(page_t *pages, uint32_t num) {
    pages->compound_num = 0;

    page_t *master = pages;
    while(num) {
        uint32_t block_size = 1ULL << log2(num);
        BUG_ON(num == master->order);
        num ^= block_size;
        free_page(master);
        master += block_size;
    }
}

static void claim_page(uint32_t idx) {
    pages[idx].flags &= ~PAGE_FLAG_PERM;
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

    for(uint32_t i = 0; i <= MAX_ORDER; i++) {
        list_init(&free_page_list[i]);
    }

    //preserve the mmap_length struct
    mmap_length = multiboot_info->mmap_length;
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

    //Replace the boot page table with the init one, and map in the page array
    mmu_init(kernel_end, malloc_start);

    multiboot_info = (void *) map_page((phys_addr_t) multiboot_info);
    if(mods) {
        mods = multiboot_info->mods = (void *) map_page((phys_addr_t) mods);
    }

    for (uint32_t i = 0; i < mods_count; i++) {
        uint32_t start = mods[i].start;
        uint32_t end = mods[i].end;
        uint32_t len = end - start;
        uint32_t num_pages = DIV_UP(end - start, PAGE_SIZE);

        mods[i].start = (uint32_t) map_pages(start, num_pages);
        mods[i].end = mods[i].start + len;
    }

    //this may arbitrarily corrupt multiboot data structures, including the mmap
    //structure, which caused a bug
    for (uint32_t page = 0; page < NUM_ENTRIES * NUM_ENTRIES; page++) {
        pages[page].flags = PAGE_FLAG_PERM | PAGE_FLAG_USED;
        pages[page].order = 0;
    }

    uint32_t malloc_page_end = DIV_DOWN(malloc_start, PAGE_SIZE) + MALLOC_NUM_PAGES;

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

            for (uint32_t j = start; j < end; j++) {
                if (j >= malloc_page_end) {
                    claim_page(j);
                }
            }
        }
    }

    pages_in_use = 0;

    kprintf("mm - paging: %u MB, malloc: %u MB, avaliable: %u MB",
            DIV_DOWN(DIV_UP(sizeof (uint32_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
            DIV_DOWN(DIV_UP(sizeof (page_t) * NUM_ENTRIES * NUM_ENTRIES, PAGE_SIZE) * PAGE_SIZE, 1024 * 1024),
            DIV_DOWN(pages_avaliable * PAGE_SIZE, 1024 * 1024));
}

void mm_postinit_reclaim() {
    uint32_t pages = DIV_UP(((uint32_t) &init_mem_end) - ((uint32_t) &init_mem_start), PAGE_SIZE);

    kprintf("mm - reclaiming %u init pages (0x%X-0x%X)", pages, &init_mem_start, &init_mem_end);

    uint32_t page_start = ((uint32_t) &init_mem_start) / PAGE_SIZE;
    for(uint32_t i = 0; i < pages; i++) {
        claim_page(page_start + i);
    }
}

void * kmalloc(uint32_t size) {
    if(size <= KALLOC_CACHE_MAX) {
        void *mem = kalloc_cache_alloc(size);
        BUG_ON(!(virt_to_page(mem)->flags & PAGE_FLAG_CACHE));
        return mem;
    } else {
        uint32_t num_pages = DIV_UP(size, PAGE_SIZE);
        page_t *pages = alloc_pages(num_pages, ALLOC_COMPOUND);
        return page_to_virt(pages);
    }
}

void kfree(void *mem) {
    if(unlikely(!mem)) return;

    page_t *first = virt_to_page(mem);
    if(first->flags & PAGE_FLAG_CACHE) {
        kalloc_cache_free(mem);
    } else {
        BUG_ON(!first->compound_num);
        //TODO unmap the pages
        free_pages(first, first->compound_num);
    }
}
