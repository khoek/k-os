#include <stdint.h>
#include "multiboot.h"
#include "elf.h"
#include "panic.h"
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "mm.h"
#include "pci.h"
#include "keyboard.h"

#define PIT_FREQ 100

void kmain(uint32_t magic, multiboot_info_t *mbd) {
    elf_init(mbd);

    console_clear();
    
    kprintf("Starting K-OS... %X %X %X\n\n", MULTIBOOT_BOOTLOADER_MAGIC, magic, mbd);
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)	panic("multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEMORY))	panic("multiboot loader did not pass memory information");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))	panic("multiboot loader did not pass memory map");

    gdt_init();

    kprintf("Registering PS/2 Keyboard Driver...\n\n");
    keyboard_init();

    kprintf("Initializing PIT to %uHz...\n\n", PIT_FREQ);
    pit_init(PIT_FREQ);

    idt_init();

    kprintf("Initializing MM...\n");

    uint32_t highest_module = 0;
    multiboot_module_t *mods = (multiboot_module_t *) mbd->mods_addr;
    for(uint32_t i = 0; i < mbd->mods_count; i++) {
        if(mods[i].mod_end > highest_module) {
            highest_module = mods[i].mod_end;
        }
    }
    mm_init((multiboot_memory_map_t *) mbd->mmap_addr, mbd->mmap_length, highest_module);

    kprintf("\nProbing PCI...\n");
    pci_init();

    kprintf("\nExploding...");
    __asm__ volatile("int $0x80");

    panic("kmain returned!");
}
