#include "common/types.h"
#include "common/compiler.h"
#include "init/multiboot.h"
#include "mm/mm.h"
#include "mm/module.h"
#include "fs/rootramfs.h"

#define MAX_MODULES 256

typedef struct module {
    uint32_t start;
    uint32_t end;
    uint32_t cmdline;
} module_t;

static __initdata uint32_t module_count;
static __initdata module_t modules[MAX_MODULES];

__init void module_init() {
    module_count = MIN(MAX_MODULES, mbi->mods_count);

    kprintf("module - detected %u module(s)", module_count);

    for(uint32_t i = 0; i < module_count; i++) {
        modules[i].start = mbi->mods[i].start;
        modules[i].end = mbi->mods[i].end;
        modules[i].cmdline = mbi->mods[i].cmdline;

        kprintf("module - #%u @ %X-%X", i + 1, modules[i].start, modules[i].end);
    }
}

static inline uint32_t get_page(uint32_t phys) {
    return phys / PAGE_SIZE;
}

__init bool is_module_page(uint32_t idx) {
    for(uint32_t i = 0; i < module_count; i++) {
        if(idx >= get_page(modules[i].start)
            && idx <= get_page(modules[i].end)) {
            return true;
        }
    }
    return false;
}

__init void module_load() {
    kprintf("module - loading modules", module_count);

    for(uint32_t i = 0; i < module_count; i++) {
        kprintf("module - #%u loaded", i + 1);
        uint32_t num_pages = DIV_UP(modules[i].end - modules[i].start, PAGE_SIZE);
        void *virt = map_pages(modules[i].start, num_pages);
        rootramfs_load(virt, modules[i].end - modules[i].start);
        //TODO unmap pages
    }

    uint32_t freed_pages = 0;

    for(uint32_t i = 0; i < module_count; i++) {
        uint32_t first_page = DIV_UP(modules[i].start, PAGE_SIZE);
        uint32_t last_page = DIV_DOWN(modules[i].end, PAGE_SIZE);
        claim_pages(first_page, last_page - first_page);
        freed_pages += last_page - first_page;
    }

    kprintf("module - %u pages reclaimed", freed_pages);
}
