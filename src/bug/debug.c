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

const char *strtab;
uint32_t strtabsz;
const elf_symbol_t *symtab;
uint32_t symtabsz;

void debug_map_virtual() {
    //FIXME this assumes that the pages will be mapped contiguously, which may not be the case
    if (strtabsz) {
        strtab = (const char *) mm_map(strtab);

        for(uint32_t offset = PAGE_SIZE; offset <= (DIV_UP(strtabsz, PAGE_SIZE) * PAGE_SIZE); offset += PAGE_SIZE) {
            mm_map((void *) (((uint32_t) strtab) + offset));
        }
    }

    if (symtabsz) {
        symtab = (const elf_symbol_t *) mm_map(symtab);

        for(uint32_t offset = PAGE_SIZE; offset <= (DIV_UP(symtabsz, PAGE_SIZE) * PAGE_SIZE); offset += PAGE_SIZE) {
            mm_map((void *) (((uint32_t) symtab) + offset));
        }
    }
}

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
