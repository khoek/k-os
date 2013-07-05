#include <stddef.h>
#include <stdbool.h>

#include "string.h"
#include "init.h"
#include "debug.h"
#include "multiboot.h"
#include "mm.h"
#include "log.h"

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
} __attribute__((packed)) elf_section_header_t;

static const char *strtab;
static uint32_t strtabsz;
static const elf_symbol_t *symtab;
static uint32_t symtabsz;

const char * debug_symbol_name(const elf_symbol_t *symbol) {
    if (symbol == NULL) return NULL;
    return (const char *) ((uint32_t) strtab + symbol->name);
}

const elf_symbol_t * debug_lookup_symbol(uint32_t address) {
    if (!strtabsz || !symtabsz) return NULL;

    for (uint32_t i = 0; i < (symtabsz / sizeof (elf_symbol_t)); i++) {
        if (ELF32_ST_TYPE(symtab[i].info) == ELF_TYPE_FUNC && address > symtab[i].value && address <= symtab[i].value + symtab[i].size) {
            return &symtab[i];
        }
    }

    return NULL;
}

void debug_map_virtual() {
    //FIXME this assumes that the pages will be mapped contiguously, which may not be the case
    if(strtabsz && symtabsz) {
        const char *strtab_reloc = mm_map(strtab);
        for(uint32_t i = PAGE_SIZE; i < strtabsz; i += PAGE_SIZE) {
            mm_map((void *)(((uint32_t) strtab) + i));
        }
        strtab = strtab_reloc;

        const elf_symbol_t *symtab_reloc = mm_map(symtab);
        for(uint32_t i = PAGE_SIZE; i < symtabsz; i += PAGE_SIZE) {
            mm_map((void *)(((uint32_t) symtab) + i));
        }
        symtab = symtab_reloc;
    }
}

void debug_init() {
    elf_section_header_t *sh = (elf_section_header_t *) multiboot_info->u.elf_sec.addr;

    for (uint32_t i = 0; i < multiboot_info->u.elf_sec.num; i++) {
        const char *name = (const char *) sh[multiboot_info->u.elf_sec.shndx].addr + sh[i].name;
        if (!strcmp(name, ".strtab")) {
            strtab = (const char *) sh[i].addr;
            strtabsz = sh[i].size;
        } else if (!strcmp(name, ".symtab")) {
            symtab = (elf_symbol_t*) sh[i].addr;
            symtabsz = sh[i].size;
        }
    }

    if (strtabsz) {
        logf("debug - strtab is present (0x%p)", strtab);
    } else {
        logf("debug - strtab is not present");
    }

    if (symtabsz) {
        logf("debug - symtab is present (0x%p)", symtab);
    } else {
        logf("debug - symtab is not present");
    }
}
