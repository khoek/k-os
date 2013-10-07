#include "lib/int.h"
#include "init/multiboot.h"
#include "init/initcall.h"
#include "init/param.h"
#include "common/version.h"
#include "common/compiler.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "arch/registers.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "task/task.h"
#include "video/log.h"
#include "video/console.h"

multiboot_info_t *multiboot_info;

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    multiboot_info = mbd;

    console_clear();

    logf("starting K-OS (v" XSTR(MAJOR) "." XSTR(MINOR) "." XSTR(PATCH) ")");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)    panic("Kernel Boot Failure - multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) panic("Kernel Boot Failure - multiboot loader did not pass memory map");

#ifndef CONFIG_OPTIMIZE
    debug_init();
#endif

    parse_cmdline();

    mm_init();
    cache_init();

    logf("starting initcalls");
    for(initcall_t *initcall = &initcall_start; initcall < &initcall_end; initcall++) {
        if((*initcall)()) panic("Kernel Boot Failure - initcall aborted with non-zero exit code");
    }
    logf("finished initcalls");

    mm_postinit_reclaim();

    logf("entering usermode");
    task_run();

    panic("kmain returned!");
}
