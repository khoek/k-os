#include "lib/int.h"
#include "init/multiboot.h"
#include "init/initcall.h"
#include "init/param.h"
#include "common/version.h"
#include "common/compiler.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "arch/proc.h"
#include "sched/proc.h"
#include "sched/sched.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "video/log.h"
#include "video/console.h"

multiboot_info_t *multiboot_info;

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    multiboot_info = mbd;

    console_clear();

    logf("starting K-OS (v" XSTR(MAJOR) "." XSTR(MINOR) "." XSTR(PATCH) ")");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)    panic("init - multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) panic("init - multiboot loader did not pass memory map");

#ifndef CONFIG_OPTIMIZE
    debug_init();
#endif

    parse_cmdline();

    mm_init();
    cache_init();

    logf("init - starting initcalls");
    for(initcall_t *initcall = &initcall_start; initcall < &initcall_end; initcall++) {
        if((*initcall)()) panic("init - initcall aborted with non-zero exit code");
    }
    logf("init - finished initcalls");

    mm_postinit_reclaim();

    sched_run();
}
