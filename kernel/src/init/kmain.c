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
#include "fs/vfs.h"
#include "fs/type/devfs.h"
#include "driver/console/console.h"

multiboot_info_t *multiboot_info;

static void umain() {
    kprintf("init - welcome to userland");

    //TODO try to execve
    // /sbin/init
    // /etc/init
    // /bin/init
    // /bin/sh
    // in that order

    while(1);
}

static void user_init() {
    path_t out, start = ROOT_PATH(root_mount);
    char *str = devfs_get_strpath("tty");
    if(!vfs_lookup(&start, str, &out)) panicf("init - cannot open tty (%s)", str);
    kfree(str, strlen(str) + 1);

    file_t *stdin = gfdt_get(vfs_open_file(out.dentry->inode));
    file_t *stdout = gfdt_get(vfs_open_file(out.dentry->inode));
    file_t *stderr = gfdt_get(vfs_open_file(out.dentry->inode));

    void *stack = page_to_virt(alloc_pages(4, 0));
    task_t *init = task_create("init", true, umain, stack, stdin, stdout, stderr);

    task_add(init);
}

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    multiboot_info = mbd;

    kprintf("starting K-OS (v" XSTR(MAJOR) "." XSTR(MINOR) "." XSTR(PATCH) ")");
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
        panic("init - multiboot loader did not pass correct magic number");
    if(!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))
        panic("init - multiboot loader did not pass memory map");

#ifndef CONFIG_OPTIMIZE
    debug_init();
#endif

    load_cmdline();

    mm_init();
    cache_init();

    parse_cmdline();

    kprintf("init - starting initcalls");
    for(initcall_t *initcall = &initcall_start; initcall < &initcall_end; initcall++) {
        if((*initcall)()) panic("init - initcall aborted with non-zero exit code");
    }
    kprintf("init - finished initcalls");

    mm_postinit_reclaim();

    user_init();

    sched_loop();
}
