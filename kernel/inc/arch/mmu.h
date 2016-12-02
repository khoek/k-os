#ifndef KERNEL_ARCH_MMU_H
#define KERNEL_ARCH_MMU_H

#define NUM_ENTRIES 1024
#define KERNEL_NUM_TABLES 256

#define MMUFLAG_PRESENT     (1 << 0)
#define MMUFLAG_WRITABLE    (1 << 1)
#define MMUFLAG_USER        (1 << 2)

#include "common/types.h"

typedef struct pdir_ent pdir_entry_t;
typedef struct pdir pdir_t;
typedef struct ptab_ent ptab_entry_t;
typedef struct ptab ptab_t;

typedef uint32_t phys_addr_t;

static inline uint32_t phys_to_pageidx(phys_addr_t addr);

#include "init/initcall.h"
#include "common/compiler.h"
#include "mm/mm.h"

static inline uint32_t phys_to_pageidx(phys_addr_t addr) {
   return addr / PAGE_SIZE;
}

static inline page_t * phys_to_page(phys_addr_t addr);

static inline phys_addr_t page_to_phys(page_t *page) {
    return get_index(page) * PAGE_SIZE;
}

static inline void * phys_to_virt(phys_addr_t phys) {
    return page_to_virt(phys_to_page(phys));
}

struct pdir_ent {
    uint32_t flags : 12;
    uint32_t physaddr : 20;
} PACKED;

struct pdir {
    pdir_entry_t e[NUM_ENTRIES];
} PACKED;

struct ptab_ent {
    uint32_t flags : 12;
    uint32_t physaddr : 20;
} PACKED;

struct ptab {
    ptab_entry_t e[NUM_ENTRIES];
} PACKED;

extern ptab_t kptab[KERNEL_NUM_TABLES];

static inline phys_addr_t virt_to_phys(void *addr);
static inline page_t * virt_to_page(void *addr);

static inline uint32_t addr_to_diridx(void *addr) {
    uint32_t raw = ((uint32_t) addr) & 0xFFFFF000;
    return (raw / PAGE_SIZE) / NUM_ENTRIES;
}

static inline uint32_t addr_to_tabidx(void *addr) {
    uint32_t raw = ((uint32_t) addr) & 0xFFFFF000;
    return (raw / PAGE_SIZE) % NUM_ENTRIES;
}

static inline uint32_t get_pageoff(uint32_t addr) {
    return addr & 0xFFF;
}

static inline uint32_t paddr_to_pageoff(phys_addr_t addr) {
    return get_pageoff((uint32_t) addr);
}

static inline uint32_t vaddr_to_pageoff(void *addr) {
    return get_pageoff((uint32_t) addr);
}

static inline phys_addr_t tabentry_get_flags(ptab_t *t, uint32_t idx) {
    return ((uint32_t) t->e[idx].flags) & 0xFFF;
}

static inline phys_addr_t tabentry_get_phys(ptab_t *t, uint32_t idx) {
    return ((uint32_t) t->e[idx].physaddr) << 12;
}

static inline void tabentry_set(ptab_t *t, uint32_t idx, phys_addr_t phys, uint32_t flags) {
    t->e[idx].physaddr = phys >> 12;
    t->e[idx].flags = flags & 0xFFF;
}

static inline phys_addr_t direntry_get_phys(pdir_t *d, uint32_t idx) {
    return ((uint32_t) d->e[idx].physaddr) << 12;
}

static inline ptab_t * dir_get_tab(pdir_t *d, uint32_t idx) {
    phys_addr_t phys = direntry_get_phys(d, idx);
    if(!phys) return NULL;

    return phys_to_virt(phys);
}

static inline void direntry_set(pdir_t *d, uint32_t idx, phys_addr_t phys, uint32_t flags) {
    d->e[idx].physaddr =  phys >> 12;
    d->e[idx].flags = flags & 0xFFF;
}

static inline void direntry_clear(pdir_t *t, uint32_t idx) {
    direntry_set(t, idx, 0, 0);
}

static inline ptab_t * dir_lookup_tab(pdir_t *dir, void *addr) {
    return dir_get_tab(dir, addr_to_diridx(addr));
}

static inline phys_addr_t tab_lookup_addr(ptab_t *tab, void *addr) {
    return tabentry_get_phys(tab, addr_to_tabidx(addr)) | vaddr_to_pageoff(addr);
}

static inline phys_addr_t dir_lookup_addr(pdir_t *dir, void *addr) {
    ptab_t *tab = dir_lookup_tab(dir, addr);
    if(!tab) return 0;

    return tab_lookup_addr(tab, addr);
}

static inline phys_addr_t virt_to_phys(void *addr) {
    if(!addr) return 0;
    return tab_lookup_addr(&kptab[addr_to_diridx(addr) - addr_to_diridx((void *) VIRTUAL_BASE)], addr);
}

static inline page_t * virt_to_page(void *addr) {
    return phys_to_page(virt_to_phys(addr));
}

static inline void invlpg(void *addr) {
    asm volatile("invlpg (%0)" :: "r" (addr) : "memory");
}

static inline void loadcr3(uint32_t phys) {
    asm volatile("mov %0, %%cr3" :: "a" (phys));
}

#define virt_is_valid(task, addr) ({                                          \
    ptab_t *tab;                                                              \
    if(task && ((uint32_t) addr) < VIRTUAL_BASE) {                            \
        tab = dir_get_tab(task->arch.dir, addr_to_diridx(addr));              \
    } else {                                                                  \
        tab = &kptab[addr_to_diridx(addr) - addr_to_diridx((void *) VIRTUAL_BASE)];                                                       \
    }                                                                         \
    tab && (tabentry_get_flags(tab, addr_to_tabidx(addr)) & MMUFLAG_PRESENT); \
})

#define resolve_virt(t, p) ({dir_lookup_addr(t->arch.dir, p);})

#include "sched/task.h"

void build_page_dir(pdir_t *dir);
void copy_mem(thread_t *to, thread_t *from);

void * map_page(phys_addr_t phys);
void * map_pages(phys_addr_t phys, uint32_t pages);

void user_map_page(thread_t *task, void *virt, phys_addr_t page);
void user_map_pages(thread_t *task, void *virt, phys_addr_t page, uint32_t num);

void * __init mmu_init(phys_addr_t kernel_end, phys_addr_t malloc_start);

#endif
