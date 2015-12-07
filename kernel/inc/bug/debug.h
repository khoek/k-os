#ifndef KERNEL_BUG_DEBUG_H
#define KERNEL_BUG_DEBUG_H

#define NOP do {} while(0)

#ifndef CONFIG_OPTIMIZE
    #include "bug/panic.h"
    #define BUG() panicf("Bugcheck failed in %s:%u", __FILE__, __LINE__)
#else
    #define BUG() NOP
#endif

#define BUG_ON(c) if((c)) BUG()

#include "fs/elf.h"

void debug_map_virtual();
const elf_symbol_t * debug_lookup_symbol(uint32_t address);
const char * debug_symbol_name(const elf_symbol_t *symbol);

void debug_init();

#endif
