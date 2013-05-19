#include <stdint.h>
#include "common.h"

#define CPL_KERNEL (0 << 5)
#define CPL_USER   (3 << 5)

#define CPL_KERNEL_CODE 0x08
#define CPL_KERNEL_DATA 0x10
#define CPL_USER_CODE   0x18
#define CPL_USER_DATA   0x20
#define CPL_USER_TSS    0x28

typedef struct tss {
   uint32_t prev_tss;   // Obsolete
   uint32_t esp0;       // Stack pointer
   uint32_t ss0;        // Stack Segment
   uint32_t esp1;       // Obsolete from here down
   uint32_t ss1;
   uint32_t esp2;
   uint32_t ss2;
   uint32_t cr3;
   uint32_t eip;
   uint32_t eflags;
   uint32_t eax;
   uint32_t ecx;
   uint32_t edx;
   uint32_t ebx;
   uint32_t esp;
   uint32_t ebp;
   uint32_t esi;
   uint32_t edi;
   uint32_t es;
   uint32_t cs;
   uint32_t ss;
   uint32_t ds;
   uint32_t fs;
   uint32_t gs;
   uint32_t ldt;
   uint16_t trap;
   uint16_t iomap_base;
} PACKED tss_t;

void gdt_init();

void gdt_set_kernel_stack(void *esp);
