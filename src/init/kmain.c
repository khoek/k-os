#include "int.h"

#include "common.h"
#include "multiboot.h"
#include "version.h"
#include "init.h"
#include "log.h"
#include "console.h"
#include "panic.h"
#include "pit.h"
#include "task.h"

multiboot_info_t *multiboot_info;

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    multiboot_info = mbd;

    console_clear();

    logf("starting K-OS (v" XSTR(MAJOR) "." XSTR(MINOR) "." XSTR(PATCH) ")");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)    panic("Kernel Boot Failure - multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) panic("Kernel Boot Failure - multiboot loader did not pass memory map");

    logf("running initcalls");
    for(initcall_t *initcall = &initcall_start; initcall < &initcall_end; initcall++) {
        if((*initcall)()) panic("Kernel Boot Failure - initcall aborted with non-zero exit code");
    }

    logf("entering usermode");
    task_switch();

    while(1);

    panic("kmain returned!");
}
