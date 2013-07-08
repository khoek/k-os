#include "lib/int.h"

#include "common/init.h"
#include "boot/module.h"
#include "fs/binfmt.h"
#include "video/log.h"

static INITCALL module_init() {
    uint32_t count = multiboot_info->mods_count;
    multiboot_module_t *mods = multiboot_info->mods;

    logf("module - detected %u module(s)", count);

    for(uint32_t i = 0; i < count; i++) {
        logf("binfmt returned %d", binfmt_load_exe((void *) mods[i].start, mods[i].end - mods[i].start));
    }

    return 0;
}

module_initcall(module_init);
