#include "common/types.h"
#include "common/compiler.h"
#include "sync/spinlock.h"
#include "bug/debug.h"
#include "arch/proc.h"
#include "arch/mmu.h"
#include "arch/bios.h"
#include "sched/task.h"

pdir_t init_page_directory ALIGN(PAGE_SIZE);
ptab_t kptab[KERNEL_NUM_TABLES] ALIGN(PAGE_SIZE);

//page which will next be mapped to kernel addr space on alloc
static uint32_t kernel_next_page;
static DEFINE_SPINLOCK(map_lock);

static inline phys_addr_t do_user_get_page(thread_t *task, uint32_t diridx, uint32_t tabidx) {
    ptab_t *tab = dir_get_tab(task->arch.dir, diridx);
    return tab ? tabentry_get_phys(tab, tabidx) : 0;
}

page_t * user_get_page(thread_t *task, void *virt) {
    phys_addr_t phys = do_user_get_page(task, addr_to_diridx(virt), addr_to_tabidx(virt));
    return phys ? phys_to_page(phys) : NULL;
}

static inline void do_user_map_page(thread_t *task, uint32_t diridx, uint32_t tabidx, phys_addr_t phys) {
    pdir_t *dir = task->arch.dir;
    ptab_t *tab = dir_get_tab(dir, diridx);
    if(!tab) {
        page_t *table_page = alloc_page(ALLOC_ZERO);
        phys_addr_t tab_phys = page_to_phys(table_page);

        direntry_set(dir, diridx, tab_phys, MMUFLAG_PRESENT | MMUFLAG_WRITABLE | MMUFLAG_USER);
        tab = page_to_virt(table_page);
    }

    tabentry_set(tab, tabidx, phys, MMUFLAG_PRESENT | MMUFLAG_WRITABLE | MMUFLAG_USER);
}

void user_map_page(thread_t *task, void *virt, phys_addr_t phys) {
    do_user_map_page(task, addr_to_diridx(virt), addr_to_tabidx(virt), phys);

    if(task == current) {
        invlpg(virt);
    }
}

void user_map_pages(thread_t *task, void *virt, phys_addr_t phys, uint32_t num) {
    for(uint32_t i = 0; i < num; i++) {
        uint32_t off = i * PAGE_SIZE;
        user_map_page(task, virt + off, phys + off);
    }
}

page_t * user_alloc_page(thread_t *task, void *virt, uint32_t flags) {
    page_t *page = alloc_page(flags);
    user_map_page(current, virt, page_to_phys(page));
    return page;
}

void copy_mem(thread_t *to, thread_t *from) {
    for (uint32_t i = 0; i < NUM_ENTRIES - KERNEL_NUM_TABLES; i++) {
        ptab_t *tab = dir_get_tab(from->arch.dir, i);
        if(tab) {
            for (uint32_t j = 0; j < NUM_ENTRIES; j++) {
                if(tabentry_get_flags(tab, j) & MMUFLAG_PRESENT) {
                    void *dst = kmalloc(PAGE_SIZE);
                    void *src = phys_to_virt(tabentry_get_phys(tab, j));
                    memcpy(dst, src, PAGE_SIZE);

                    do_user_map_page(to, i, j, virt_to_phys(dst));
                }
            }
        }
    }
}

static inline void map_kernel_page(uint32_t page_idx, uint32_t phys) {
    uint32_t diridx = page_idx / NUM_ENTRIES;
    uint32_t tabidx = page_idx % NUM_ENTRIES;
    tabentry_set(&kptab[diridx], tabidx, phys, MMUFLAG_WRITABLE | MMUFLAG_PRESENT);
}

static inline void * get_next_virt(uint32_t phys) {
    return (void *) ((kernel_next_page * PAGE_SIZE) + VIRTUAL_BASE + paddr_to_pageoff(phys));
}

static inline void do_map_page(phys_addr_t phys) {
    void *virt = get_next_virt(0);
    kernel_next_page++;
    uint32_t idx = addr_to_diridx(virt) - addr_to_diridx((void *) VIRTUAL_BASE);
    tabentry_set(&kptab[idx], addr_to_tabidx(virt), phys, MMUFLAG_WRITABLE | MMUFLAG_PRESENT);
    invlpg(virt);
}

void * map_page(phys_addr_t phys) {
    return map_pages(phys, 1);
}

void * map_pages(phys_addr_t phys, uint32_t pages) {
    uint32_t flags;
    spin_lock_irqsave(&map_lock, &flags);

    void *virt = get_next_virt(phys);
    for(uint32_t i = 0; i < pages; i++) {
        do_map_page(phys + (PAGE_SIZE * i));
    }

    spin_unlock_irqstore(&map_lock, flags);

    return virt;
}

static inline phys_addr_t kvirt_to_phys(void *kaddr) {
    BUG_ON(((uint32_t) kaddr) < VIRTUAL_BASE);
    return ((uint32_t) kaddr) - VIRTUAL_BASE;
}

void build_page_dir(pdir_t *dir) {
    for (uint32_t i = 0; i < NUM_ENTRIES - KERNEL_NUM_TABLES; i++) {
        direntry_clear(dir, i);
    }

    uint32_t baseoff = VIRTUAL_BASE / PAGE_SIZE / NUM_ENTRIES;
    for (uint32_t i = 0; i < KERNEL_NUM_TABLES; i++) {
        phys_addr_t phys = kvirt_to_phys(&kptab[i]);
        direntry_set(dir, i + baseoff, phys, MMUFLAG_PRESENT | MMUFLAG_WRITABLE);
    }
}

void * __init mmu_init(phys_addr_t kernel_end, phys_addr_t malloc_start) {
    kernel_next_page = 0;

    //Map 0xC0000000->0x00000000, 0xC0001000->0x00001000, etc. over the whole
    //kernel image. Then map the page_t struct array just after the kernel
    //image.
    map_pages(0, DIV_UP(kernel_end, PAGE_SIZE));
    void *pages = map_pages(malloc_start, MALLOC_NUM_PAGES);

    //Build the temporary page directory for all processors.
    build_page_dir(&init_page_directory);

    //Here we go!
    loadcr3(kvirt_to_phys(&init_page_directory));

    //Immediately fix the pointers to the console and symbol tables.
    bios_early_remap();
    console_early_remap();
    debug_remap();

    return pages;
}
