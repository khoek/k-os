#include <stdint.h>

#include "init.h"
#include "module.h"
#include "binfmt.h"
#include "log.h"

static multiboot_module_t *mods;
static uint32_t count;

uint32_t module_count() {
    return count;
}

multiboot_module_t * module_get(uint32_t num) {
    return &mods[num];
}

static INITCALL module_init() {
    mods = multiboot_info->mods;
    count = multiboot_info->mods_count;

    logf("module - detected %u module(s)", count);

    for(uint32_t i = 0; i < count; i++) {
        logf("binfmt returned %d", binfmt_load_exe((void *) mods[i].start, mods[i].end - mods[i].start));
    }

    return 0;
}

module_initcall(module_init);
