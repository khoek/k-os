#ifndef KERNEL_BUG_DEBUG_H
#define KERNEL_BUG_DEBUG_H

#define UNIMPLEMENTED() do { panicf("Unimplemented! @ %s:%u", __FILE__, __LINE__); } while(0)

#define NOP() do {} while(0)

#define BUG() do { panicf("Bugcheck failed in %s:%u", __FILE__, __LINE__); } while(0)

#ifdef CONFIG_DEBUG_BUGCHECKS
    #define BUG_ON(c) do { if((c)) BUG(); } while(0)
#else
    #define BUG_ON(c) do { NOP(); (void)(c);} while(0)
#endif

#define breakpoint() do { breakpoint_triggered(); } while(0)

#include "bug/panic.h"
#include "fs/elf.h"

void breakpoint_triggered();

void debug_map_virtual();
const elf_symbol_t * debug_lookup_symbol(uint32_t address);
const char * debug_symbol_name(const elf_symbol_t *symbol);

void debug_init();

#endif
