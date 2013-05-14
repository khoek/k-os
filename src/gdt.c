#include <stddef.h>
#include "gdt.h"
#include "common.h"

typedef struct gdtr {
    unsigned short size;
    unsigned int offset;
} PACKED gdtr_t;

typedef struct gdt_entry {
    unsigned short limit_0_15;
    unsigned short base_0_15;
    unsigned char base_16_23;
    unsigned char access;
    unsigned char limit_16_19_and_flags;
    unsigned char base_24_31;
} PACKED gdt_entry_t;

gdt_entry_t* gdt = (gdt_entry_t*) 0x71000;
gdtr_t gdtr;

extern int end_of_image;

void reload_segment_registers();

void gdt_init() {
     unsigned int code_end = ((unsigned int)&end_of_image + 4095) / 4096;
     
     // 0x00 is a null selector:
     gdt[0].limit_0_15              = 0;
     gdt[0].base_0_15               = 0;
     gdt[0].base_16_23              = 0;
     gdt[0].access                  = 0;
     gdt[0].limit_16_19_and_flags   = 0;
     gdt[0].base_24_31              = 0;

     // 0x08 is a code selector:
     gdt[1].limit_0_15              = code_end & 0xffff;
     gdt[1].base_0_15               = 0x0000;
     gdt[1].base_16_23              = 0x00;
     gdt[1].access                  = 0x02 /* readable */ | 0x08 /* code segment */ | 0x10 /* reserved */ | 0x80 /* present */;
     gdt[1].limit_16_19_and_flags   = ((code_end >> 16) & 0xf) | 0x40 /* 32 bit */ | 0x80 /* 4 KiB */;
     gdt[1].base_24_31              = 0x00;
     
     // 0x10 is a data selector:
     gdt[2].limit_0_15              = 0xffff;
     gdt[2].base_0_15               = 0x0000;
     gdt[2].base_16_23              = 0x00;
     gdt[2].access                  = 0x02 /* readable */ | 0x10 /* reserved */ | 0x80 /* present */;
     gdt[2].limit_16_19_and_flags   = 0xf | 0x40 /* 32 bit */ | 0x80 /* 4 KiB */;
     gdt[2].base_24_31              = 0x00;
     
     // 0x18 is a 16 bit code selector
     gdt[3].limit_0_15              = 0xffff;
     gdt[3].base_0_15               = 0;
     gdt[3].base_16_23              = 0;
     gdt[3].access                  = 0x02 /* readable */ | 0x08 /* code segment */ | 0x10 /* reserved */ | 0x80 /* present */;
     gdt[3].limit_16_19_and_flags   = 0 /* neither 16 bit or 4 KiB */;
     gdt[3].base_24_31              = 0x00;
     
     // 0x20 is a 16 bit data selector
     gdt[4].limit_0_15              = 0xffff;
     gdt[4].base_0_15               = 0;
     gdt[4].base_16_23              = 0;
     gdt[4].access                  = 0x02 /* readable */ | 0x10 /* reserved */ | 0x80 /* present */;
     gdt[4].limit_16_19_and_flags   = 0 /* neither 16 bit or 4 KiB */;
     gdt[4].base_24_31              = 0x00;
     
     gdtr.size = (sizeof(gdt_entry_t) * 5) - 1;
     gdtr.offset = (unsigned int) gdt;
     
     __asm__ volatile("lgdt (%0)" :: "m"(gdtr));
     
     reload_segment_registers();
}
