#include <stdbool.h>

#include "init/multiboot.h"
#include "lib/string.h"
#include "init/initcall.h"
#include "common/math.h"
#include "common/compiler.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "log/log.h"

void breakpoint_triggered() {

}

typedef struct {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} PACKED elf_section_header_t;

static const char *strtab;
static uint32_t strtabsz;
static const elf_symbol_t *symtab;
static uint32_t symtabsz;

const char * debug_symbol_name(const elf_symbol_t *symbol) {
    if(symbol == NULL) return NULL;
    return (const char *) ((uint32_t) strtab + symbol->name);
}

const elf_symbol_t * debug_lookup_symbol(uint32_t address) {
    if(!strtabsz || !symtabsz) return NULL;

    for(uint32_t i = 0; i < (symtabsz / sizeof(elf_symbol_t)); i++) {
        if (ELF32_ST_TYPE(symtab[i].info) == ELF_TYPE_FUNC && address > symtab[i].value && address <= symtab[i].value + symtab[i].size) {
            return &symtab[i];
        }
    }

    return NULL;
}

void __init debug_map_virtual() {
    if(strtabsz && symtabsz) {
        strtab = map_pages((phys_addr_t) strtab, DIV_UP(strtabsz, PAGE_SIZE));
        symtab = map_pages((phys_addr_t) symtab, DIV_UP(symtabsz, PAGE_SIZE));
    }
}

void __init debug_init() {
    elf_section_header_t *sh = (elf_section_header_t *) multiboot_info->u.elf_sec.addr;

    for(uint32_t i = 0; i < multiboot_info->u.elf_sec.num; i++) {
        const char *name = (const char *) sh[multiboot_info->u.elf_sec.shndx].addr + sh[i].name;
        if (!strcmp(name, ".strtab")) {
            strtab = (const char *) sh[i].addr;
            strtabsz = sh[i].size;
        } else if (!strcmp(name, ".symtab")) {
            symtab = (elf_symbol_t *) sh[i].addr;
            symtabsz = sh[i].size;
        }
    }

    if(strtabsz) {
        kprintf("debug - strtab is present (0x%p)", strtab);
    } else {
        kprintf("debug - strtab is not present");
    }

    if(symtabsz) {
        kprintf("debug - symtab is present (0x%p)", symtab);
    } else {
        kprintf("debug - symtab is not present");
    }
}
