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
    console_clear();

    kprintf("Starting K-OS...\n\n");
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

    kprintf("\n");
    for(uint8_t seconds = 5; seconds > 0; seconds--) {
        kprintf("Exploding... %u\r", seconds);
        sleep(100);
    }
    kprintf("Exploding... 0\r");
    sleep(10);
    kprintf("Exploding... BOOM!");

    __asm__ volatile("int $0x80");

    panic("kmain returned!");
}
