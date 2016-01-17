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
#include "log/log.h"
#include "driver/console/console.h"

multiboot_info_t *multiboot_info;

static void user_init() {
    kprintf("init - welcome to userland");

    //TODO try to execve
    // /sbin/init
    // /etc/init
    // /bin/init
    // /bin/sh
    // in that order

    while(1);
}

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    multiboot_info = mbd;

    kprintf("starting K-OS (v" XSTR(MAJOR) "." XSTR(MINOR) "." XSTR(PATCH) ")");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)    panic("init - multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) panic("init - multiboot loader did not pass memory map");

#ifndef CONFIG_OPTIMIZE
    debug_init();
#endif

    parse_cmdline();

    mm_init();
    cache_init();

    kprintf("init - starting initcalls");
    for(initcall_t *initcall = &initcall_start; initcall < &initcall_end; initcall++) {
        if((*initcall)()) panic("init - initcall aborted with non-zero exit code");
    }
    kprintf("init - finished initcalls");

    mm_postinit_reclaim();

    task_t *init = task_create("init", true, user_init, NULL);
    //TODO remap the STD(IN/OUT/ERR) of this process to the screen
    task_add(init);

    sched_loop();
}
