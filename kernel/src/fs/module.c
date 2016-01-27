#include "lib/int.h"
#include "lib/printf.h"
#include "init/multiboot.h"
#include "init/initcall.h"
#include "fs/binfmt.h"
#include "fs/rootramfs.h"
#include "mm/mm.h"
#include "log/log.h"

extern uint32_t mods_count;

static INITCALL module_init() {
    multiboot_module_t *mods = multiboot_info->mods;

    kprintf("module - detected %u module(s)", mods_count);

    for(uint32_t i = 0; i < mods_count; i++) {
        rootramfs_load((void *) mods[i].start, mods[i].end - mods[i].start);
        kprintf("module - #%u loaded", i + 1);
    }

    return 0;
}

module_initcall(module_init);
