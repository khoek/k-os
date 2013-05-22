#include "init.h"
#include "module.h"
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

    for(uint32_t i = 0; i < multiboot_info->mods_count; i++) {
        logf("detected module #%u at (0x%p - 0x%p) %s\n", i, mods[i].start, mods[i].end, mods[i].cmdline);
    }

    return 0;
}

module_initcall(module_init);
