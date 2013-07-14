#ifndef KERNEL_BUG_DEBUG_H
#define KERNEL_BUG_DEBUG_H

#ifndef CONFIG_OPTIMIZE

#include "bug/panic.h"
#define DEPENDS(m, c) if(!(c)) panic("Dependency assertion on \"%s\" failed!", m)
#define ASSERT(c)     if(!(c)) panic("Assertion failed!")
#define BUG_ON(c)     if((c))  panic("Bug!")
#define BUG()         panic("Bug!")

#else

#define DEPENDS(m, c)
#define ASSERT(c)
#define BUG_ON(c)
#define BUG()

#endif

#include "fs/elf.h"

void debug_map_virtual();
const elf_symbol_t * debug_lookup_symbol(uint32_t address);
const char * debug_symbol_name(const elf_symbol_t *symbol);

void debug_init();

#endif
