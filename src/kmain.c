#include "multiboot.h"
#include "panic.h"
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "rand.h"
#include "mm.h"
#include "pci.h"
#include "vfs.h"
#include "keyboard.h"
#include "shell.h"
#include "io.h"
#include "erasure_tool.h"

void kmain(multiboot_info_t *mbd, int magic) {
   console_clear();
   kprintf("Starting K-OS...\n\n");

   if (magic != MULTIBOOT_BOOTLOADER_MAGIC)	panic("multiboot loader did not pass correct magic number");
   if (!(mbd->flags & MULTIBOOT_INFO_MEMORY))	panic("multiboot loader did not pass memory information");
   if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))	panic("multiboot loader did not pass memory map");
   if (!(mbd->flags & MULTIBOOT_INFO_BOOTDEV))	panic("multiboot loader did not pass boot device");

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

   kprintf("\ndone!");

   //erasure_tool_run();
}
