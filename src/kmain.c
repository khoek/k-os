#include <stdint.h>
#include "multiboot.h"
#include "version.h"
#include "console.h"
#include "panic.h"
#include "elf.h"
#include "gdt.h"
#include "idt.h"
#include "module.h"
#include "keyboard.h"
#include "syscall.h"
#include "pit.h"
#include "mm.h"
#include "cache.h"
#include "pci.h"
#include "task.h"

#define PIT_FREQ 100

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    console_clear();

    kprintf("Starting K-OS (v%u.%u.%u)...\n\n", MAJOR, MINOR, PATCH);
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)    panic("multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) panic("multiboot loader did not pass memory map");

    elf_init(mbd);

    gdt_init();

    kprintf("Initializing MM...\n");
    mm_init(mbd);
    kprintf("\n");

    kprintf("Initializing Caches...\n");
    cache_init();
    kprintf("\n");

    cli();

    idt_init();

    kprintf("Parsing Kernel Modules...\n");
    module_init(mbd);
    kprintf("\n");

    kprintf("Initializing PS/2 Keyboard Driver...\n");
    keyboard_init();
    kprintf("\n");

    kprintf("Initializing Syscall Handlers...\n");
    syscall_init();
    kprintf("\n");

    kprintf("Initializing PIT to %uHz...\n", PIT_FREQ);
    pit_init(PIT_FREQ);
    kprintf("\n");

    sti();

    kprintf("Probing PCI...\n");
    pci_init();
    kprintf("\n");

    kprintf("Initializing Multitasking...\n");
    task_init();
    kprintf("\n");

    kprintf("Entering Usermode...\n\n");
    task_start();

    panic("kmain returned!");
}
