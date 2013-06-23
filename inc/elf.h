#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include "int.h"
#include <common.h>

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;

#define EI_NIDENT	16
#define	EI_MAG0		0
#define	EI_MAG1		1
#define	EI_MAG2		2
#define	EI_MAG3		3
#define	EI_CLASS	4
#define	EI_DATA		5
#define	EI_VERSION	6
#define	EI_OSABI	7
#define	EI_PAD		8

#define	ELFMAG0		0x7f
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"

#define	ELFCLASSNONE 0
#define	ELFCLASS32   1
#define	ELFCLASS64   2
#define	ELFCLASSNUM  3

#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_NONE	   0
#define EV_CURRENT 1

#define EM_NONE  0
#define EM_M32   1
#define EM_SPARC 2
#define EM_386   3
#define EM_68K   4
#define EM_88K   5
#define EM_860   7
#define EM_MIPS  8

typedef struct elf32_hdr {
  unsigned char	e_ident[EI_NIDENT];
  Elf32_Half	e_type;
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;
  Elf32_Off	e_phoff;
  Elf32_Off	e_shoff;
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;
  Elf32_Half	e_phentsize;
  Elf32_Half	e_phnum;
  Elf32_Half	e_shentsize;
  Elf32_Half	e_shnum;
  Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7
#define PT_LOOS    0x60000000
#define PT_HIOS    0x6fffffff
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff

#define PF_R		0x4
#define PF_W		0x2
#define PF_X		0x1

typedef struct elf32_phdr {
  Elf32_Word	p_type;
  Elf32_Off	    p_offset;
  Elf32_Addr	p_vaddr;
  Elf32_Addr	p_paddr;
  Elf32_Word	p_filesz;
  Elf32_Word	p_memsz;
  Elf32_Word	p_flags;
  Elf32_Word	p_align;
} Elf32_Phdr;

#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_INFO(b, t) (((b)<<4)+((t)&0xf))

#define ELF_TYPE_NOTYPE 0
#define ELF_TYPE_OBJECT 1
#define ELF_TYPE_FUNC 2
#define ELF_TYPE_SECTION 3
#define ELF_TYPE_FILE 4
#define ELF_TYPE_COMMON 5
#define ELF_TYPE_LOOS 10
#define ELF_TYPE_HIOS 12
#define ELF_TYPE_LOPROC 13
#define ELF_TYPE_HIPROC 15

#define ELF_BIND_LOCAL 0
#define ELF_BIND_GLOBAL 1
#define ELF_BIND_WEAK 2
#define ELF_BIND_LOOS 10
#define ELF_BIND_HIOS 12
#define ELF_BIND_LOPROC 13
#define ELF_BIND_HIPROC 15

typedef struct {
  uint32_t name;
  uint32_t value;
  uint32_t size;
  uint8_t info;
  uint8_t other;
  uint16_t shndx;
} __attribute__((packed)) elf_symbol_t;

#endif
