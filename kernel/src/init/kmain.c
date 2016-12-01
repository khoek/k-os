#include "common/types.h"
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
#include "sched/ktaskd.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "log/log.h"
#include "fs/vfs.h"
#include "fs/exec.h"
#include "fs/type/devfs.h"
#include "driver/console/console.h"

multiboot_info_t *multiboot_info;

static char *ENVP[] = {NULL};

static bool try_load_init(char *path) {
    char *argv[2];
    argv[0] = path;
    argv[1] = NULL;

    path_t out;
    if(vfs_lookup(NULL, path, &out)) {
        execute_path(&out, argv, ENVP);
    }

    return false;
}

static void umain() {
    kprintf("init - process started");

    ktaskd_init();
    kprintf("init - ktaskd created");

    path_t out;
    char *str = devfs_get_strpath("tty");
    if(!vfs_lookup(NULL, str, &out)) {
        panicf("init - cannot open tty (%s)", str);
    }
    kfree(str);

    //vlog_disable();

    ufdt_add(0, vfs_open_file(out.dentry->inode));
    ufdt_add(0, vfs_open_file(out.dentry->inode));
    ufdt_add(0, vfs_open_file(out.dentry->inode));

    if(!try_load_init("/sbin/init")
        && !try_load_init("/etc/init")
        && !try_load_init("/bin/init")
        && !try_load_init("/bin/sh")) {
        panic("init - could not load init process");
    }
}

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    multiboot_info = mbd;

    kprintf("starting K-OS (v" XSTR(MAJOR) "." XSTR(MINOR) "." XSTR(PATCH) ")");
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
        panic("main - multiboot loader did not pass correct magic number");
    if(!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))
        panic("main - multiboot loader did not pass memory map");

    debug_init();

    load_cmdline();

    mm_init();
    cache_init();

    parse_cmdline();

    kprintf("main - starting initcalls");
    for(initcall_t *initcall = &initcall_start; initcall < &initcall_end; initcall++) {
        if((*initcall)()) {
            panic("main - initcall aborted with non-zero exit code");
        }
    }
    kprintf("main - finished initcalls");

    root_task_init(umain);

    mm_postinit_reclaim();

    kprintf("main - init process handoff");
    sched_loop();
}
