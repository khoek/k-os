#include <stdint.h>
#include "multiboot.h"
#include "version.h"
#include "console.h"
#include "panic.h"
#include "elf.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "pit.h"
#include "mm.h"
#include "pci.h"

#define PIT_FREQ 100

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    console_clear();

    kprintf("Starting K-OS (v%u.%u.%u)...\n\n", MAJOR, MINOR, PATCH);
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)    panic("multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEMORY))  panic("multiboot loader did not pass memory information");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) panic("multiboot loader did not pass memory map");

    elf_init(mbd);

    gdt_init();
    idt_init();

    kprintf("Registering PS/2 Keyboard Driver...\n\n");
    keyboard_init();

    kprintf("Initializing PIT to %uHz...\n\n", PIT_FREQ);
    pit_init(PIT_FREQ);

    kprintf("Initializing MM...\n");
    mm_init(mbd);

    kprintf("\nProbing PCI...\n");
    pci_init();

    kprintf("\nDone.");
    
    die();
    panic("kmain returned!");
}

