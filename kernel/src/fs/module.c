#include "lib/int.h"
#include "lib/printf.h"
#include "init/initcall.h"
#include "fs/module.h"
#include "fs/binfmt.h"
#include "video/log.h"

static INITCALL module_init() {
    uint32_t count = multiboot_info->mods_count;
    multiboot_module_t *mods = multiboot_info->mods;

    logf("module - detected %u module(s)", count);

    char buff[64];
    for(uint32_t i = 0; i < count; i++) {
        sprintf(buff, "module%d", i + 1);
        logf("module - #%u load %s", i + 1, binfmt_load_exe(buff, (void *) mods[i].start, mods[i].end - mods[i].start) ? "FAIL" : "OK");
    }

    return 0;
}

module_initcall(module_init);
