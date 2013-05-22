#include "init.h"
#include "module.h"
#include "console.h"

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
        kprintf("    - Module #%u at (0x%08X - 0x%08X) %s\n", i, mods[i].start, mods[i].end, mods[i].cmdline);
    }

    return 0;
}

module_initcall(module_init);
