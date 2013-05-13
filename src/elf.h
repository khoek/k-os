#include <stdint.h>
#include "multiboot.h"

#define ELF32_ST_TYPE(i)    ((i) & 0xf)
#define ELF32_ST_BIND(i)    ((i) >> 4)
#define ELF32_ST_INFO(b, t) (((b)<<4)+((t)&0xf))

#define ELF_TYPE_NOTYPE      0
#define ELF_TYPE_OBJECT      1
#define ELF_TYPE_FUNC        2
#define ELF_TYPE_SECTION     3
#define ELF_TYPE_FILE        4
#define ELF_TYPE_COMMON      5
#define ELF_TYPE_LOOS       10
#define ELF_TYPE_HIOS       12
#define ELF_TYPE_LOPROC     13
#define ELF_TYPE_HIPROC     15

#define ELF_BIND_LOCAL      0
#define ELF_BIND_GLOBAL     1
#define ELF_BIND_WEAK       2
#define ELF_BIND_LOOS       10
#define ELF_BIND_HIOS       12
#define ELF_BIND_LOPROC     13
#define ELF_BIND_HIPROC     15 

typedef struct {
  uint32_t name;
  uint32_t value;
  uint32_t size;
  uint8_t  info;
  uint8_t  other;
  uint16_t shndx;
} __attribute__((packed)) elf_symbol_t;

void elf_init(multiboot_info_t *mbd);
elf_symbol_t * elf_lookup_symbol(uint32_t address);
const char * elf_symbol_name(elf_symbol_t *symbol);
