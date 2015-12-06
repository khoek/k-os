#include "lib/int.h"
#include "lib/printf.h"
#include "init/initcall.h"
#include "fs/module.h"
#include "fs/binfmt.h"
#include "video/log.h"

extern uint32_t mods_count;

static INITCALL module_init() {
    multiboot_module_t *mods = multiboot_info->mods;

    kprintf("module - detected %u module(s)", mods_count);

    char buff[64];
    for(uint32_t i = 0; i < mods_count; i++) {
        sprintf(buff, "module%d", i + 1);
        kprintf("module - #%u load %s", i + 1, binfmt_load_exe(buff, (void *) mods[i].start, mods[i].end - mods[i].start) ? "FAIL" : "OK");
    }

    return 0;
}

module_initcall(module_init);
