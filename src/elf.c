#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "elf.h"

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

typedef struct {
  const char   *strtab;
  uint32_t      strtabsz;
  bool          strtabfound;
  elf_symbol_t *symtab;
  uint32_t      symtabsz;
  bool          symtabfound;
} kernel_t;

static kernel_t kernel;

void elf_init(multiboot_info_t *mbd) {
    elf_section_header_t *sh = (elf_section_header_t *) mbd->u.elf_sec.addr;

    for (uint32_t i = 0; i < mbd->u.elf_sec.num; i++) {
        const char *name = (const char *) sh[mbd->u.elf_sec.shndx].addr + sh[i].name;
        if (!strcmp(name, ".strtab")) {
            kernel.strtab = (const char *) sh[i].addr;
            kernel.strtabsz = sh[i].size;
            kernel.strtabfound = true;
        } else if (!strcmp(name, ".symtab")) {
            kernel.symtab = (elf_symbol_t*) sh[i].addr;
            kernel.symtabsz = sh[i].size;
            kernel.symtabfound = true;
        }
    }
}

elf_symbol_t * elf_lookup_symbol(uint32_t address) {
    if(!kernel.strtabfound || !kernel.symtabfound) return NULL;

    for (uint32_t i = 0; i < (kernel.symtabsz / sizeof(elf_symbol_t)); i++) {
        if (ELF32_ST_TYPE(kernel.symtab[i].info) == ELF_TYPE_FUNC && address > kernel.symtab[i].value && address <= kernel.symtab[i].value + kernel.symtab[i].size) {
            return &kernel.symtab[i];
        }
    }

    return NULL;
}

const char * elf_symbol_name(elf_symbol_t *symbol) {
    if(symbol == NULL) return NULL;
    return (const char *) ((uint32_t) kernel.strtab + symbol->name);
}
