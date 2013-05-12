#include "multiboot.h"
#include "panic.h"
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "rand.h"
#include "mm.h"
#include "ide.h"
#include "pci.h"
#include "vfs.h"
#include "keyboard.h"
#include "shell.h"
#include "io.h"
#include "erasure_tool.h"

extern int symtab_start;
extern int symtab_end;
extern int end_of_image;

void kmain(multiboot_info_t *mbd, int magic) {
    console_clear();
    kprintf("Starting K-OS...\n\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)	panic("multiboot loader did not pass correct magic number");
    if (!(mbd->flags & MULTIBOOT_INFO_MEMORY))	panic("multiboot loader did not pass memory information");
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))	panic("multiboot loader did not pass memory map");

    gdt_init();

    kprintf("Registering PS/2 Keyboard Driver...\n\n");
    keyboard_init();

    idt_init();

    pit_init(100);

    kprintf("Initializing MM...\n");

    uint32_t highest_module = 0;
    multiboot_module_t *mods = (multiboot_module_t *) mbd->mods_addr;
    for(uint32_t i = 0; i < mbd->mods_count; i++) {
     if(mods[i].mod_end > highest_module) {
      highest_module = mods[i].mod_end;
     }
    }
    mm_init((multiboot_memory_map_t *) mbd->mmap_addr, mbd->mmap_length, highest_module);

    kprintf("\nProbing PCI... %d\n");
    pci_init();

    //kprintf("\nInitializing VFS...\n");
    //vfs_init(mbd->boot_device);

    //kprintf("\n\nLaunching Shell Command Interpreter...");
    //run_shell();

    //sleep(100);

    //console_clear();

    for(uint8_t i = 0; i < 4; i++) {
       kprintf("%c", ide_device_is_present(i) ? 'X' : '-');
    }
    kprintf("\n\n");

    unsigned char box[512];
    ide_read_sectors(0, 1, 0, box);

    for(int i = 0; i < 16; i++) {
      kprintf("%02X%02X ", box[i * 2], box[i * 2 + 1]);
    }

    kprintf("kernel symtab 0x%p-0x%p,\nimage ends at 0x%p\n%p\n", &symtab_start, &symtab_end, &end_of_image, mbd->u.elf_sec);

    //__asm__ volatile("jmp *%0" :: "p" (box));
    __asm__ volatile("int $0x80");

    kprintf("\ndone!");
}
