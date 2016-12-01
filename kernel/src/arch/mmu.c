#include "common/types.h"
#include "common/compiler.h"
#include "sync/spinlock.h"
#include "bug/debug.h"
#include "arch/proc.h"
#include "arch/mmu.h"
#include "sched/task.h"

pdir_t init_page_directory ALIGN(PAGE_SIZE);
ptab_t kptab[KERNEL_NUM_TABLES] ALIGN(PAGE_SIZE);

//page which will next be mapped to kernel addr space on alloc
static uint32_t kernel_next_page;
static DEFINE_SPINLOCK(map_lock);

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

static void * do_map_page(phys_addr_t phys) {
    void *virt = (void *) ((kernel_next_page++ * PAGE_SIZE) + VIRTUAL_BASE);
    uint32_t idx = addr_to_diridx(virt) - addr_to_diridx((void *) VIRTUAL_BASE);
    tabentry_set(&kptab[idx], addr_to_tabidx(virt), phys, MMUFLAG_WRITABLE | MMUFLAG_PRESENT);
    invlpg(virt);

    return virt + paddr_to_pageoff(phys);
}

void * map_page(phys_addr_t phys) {
    return map_pages(phys, 1);
}

void * map_pages(phys_addr_t phys, uint32_t pages) {
    uint32_t flags;
    spin_lock_irqsave(&map_lock, &flags);

    void *virt = do_map_page(phys);
    for(uint32_t i = 1; i < pages; i++) {
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

void task_mmu_setup(thread_t *task) {
    build_page_dir(task->arch.dir);
}

void __init mmu_init(phys_addr_t kernel_end, phys_addr_t malloc_start) {
    uint32_t kernel_num_pages = DIV_UP(kernel_end, PAGE_SIZE);

    //Map 0xC0000000->0x00000000, 0xC0001000->0x00001000, etc. over the whole
    //kernel image.
    for(uint32_t page_idx = 0; page_idx < kernel_num_pages; page_idx++) {
        uint32_t diridx = page_idx / NUM_ENTRIES;
        uint32_t tabidx = page_idx % NUM_ENTRIES;
        phys_addr_t phys = page_idx * PAGE_SIZE;

        tabentry_set(&kptab[diridx], tabidx, phys, MMUFLAG_WRITABLE | MMUFLAG_PRESENT);
    }

    //Map the page_t struct array just after the kernel image.
    for(uint32_t i = 0; i < MALLOC_NUM_PAGES; i++) {
        uint32_t page_idx = kernel_num_pages + i;
        uint32_t diridx = page_idx / NUM_ENTRIES;
        uint32_t tabidx = page_idx % NUM_ENTRIES;
        phys_addr_t phys = malloc_start + (i * PAGE_SIZE);

        tabentry_set(&kptab[diridx], tabidx, phys, MMUFLAG_WRITABLE | MMUFLAG_PRESENT);
    }

    kernel_next_page = kernel_num_pages + MALLOC_NUM_PAGES;

    build_page_dir(&init_page_directory);

    loadcr3(kvirt_to_phys(&init_page_directory));

    debug_map_virtual();
}
