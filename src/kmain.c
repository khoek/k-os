#include <stdint.h>
#include "multiboot.h"
#include "version.h"
#include "init.h"
#include "idt.h"
#include "console.h"
#include "log.h"
#include "pit.h"
#include "panic.h"
#include "task.h"

multiboot_info_t *multiboot_info;

extern initcall_t initcall_start, initcall_end;

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    multiboot_info = mbd;

    console_clear();

    logf("starting K-OS (v%u.%u.%u)", MAJOR, MINOR, PATCH);
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)    panic("multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) panic("multiboot loader did not pass memory map");

    logf("running initcalls");
    for(initcall_t *initcall = &initcall_start; initcall < &initcall_end; initcall++) {
        (*initcall)();
    }

    logf("entering usermode");
    task_start();

    panic("kmain returned!");
}
