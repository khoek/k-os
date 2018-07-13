#ifndef KERNEL_BUG_DEBUG_H
#define KERNEL_BUG_DEBUG_H

#include "bug/panic.h"
#include "mm/mm.h"
#include "fs/elf.h"
#include "driver/console/console.h"

void breakpoint_triggered();

void dump_stack_trace(console_t *t);

void debug_remap();
const elf_symbol_t * debug_lookup_symbol(uint32_t address);
const char * debug_symbol_name(const elf_symbol_t *symbol);

phys_addr_t __init debug_kernel_start(phys_addr_t raw);
phys_addr_t __init debug_kernel_end(phys_addr_t raw);

void debug_init();

#endif
