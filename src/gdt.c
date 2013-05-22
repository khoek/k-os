#include <stddef.h>
#include "string.h"
#include "init.h"
#include "gdt.h"
#include "idt.h"
#include "console.h"
#include "panic.h"
#include "registers.h"

#define GDT_SIZE 6

typedef struct gdtd {
    uint16_t size;
    uint32_t offset;
} PACKED gdtd_t;

typedef struct gdt_entry {
    uint16_t limit_0_15;
    uint16_t base_0_15;
    uint8_t  base_16_23;
    uint8_t  access;
    uint8_t  limit_16_19_and_flags;
    uint8_t  base_24_31;
} PACKED gdt_entry_t;

static gdtd_t gdtd;
static gdt_entry_t gdt[GDT_SIZE];
static tss_t tss;

#define PRESENT     (1 << 7)
#define TSS         (1 << 0)
#define SEGMENT     (1 << 4)
#define EXECABLE    (1 << 3)
#define WRITABLE    (1 << 1)

#define FLAG_AVAILABLE        (1 << 4)
#define FLAG_BITS_32          (1 << 6)
#define FLAG_GRANULARITY_BYTE (0 << 7)
#define FLAG_GRANULARITY_PAGE (1 << 7)

void set_kernel_stack(void *esp) {
    tss.esp0 = (uint32_t) esp;
}

static void create_selector(uint16_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[index].limit_0_15 = limit & 0xFFFF;
    gdt[index].limit_16_19_and_flags = flags | ((limit >> 16) & 0xF);
    gdt[index].base_0_15  = base & 0xFFFF;
    gdt[index].base_16_23 = (base >> 16) & 0x00FF;
    gdt[index].base_24_31 = base >> 24;
    gdt[index].access     = access;
}

static INITCALL gdt_init() {
    create_selector(0, 0x00000000, 0x00000, 0, 0);
    create_selector(1, 0x00000000, 0xFFFFF,            PRESENT | CPL_KERNEL | WRITABLE | EXECABLE | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE);
    create_selector(2, 0x00000000, 0xFFFFF,            PRESENT | CPL_KERNEL | WRITABLE            | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE);
    create_selector(3, 0x00000000, 0xFFFFF,            PRESENT | CPL_USER   | WRITABLE | EXECABLE | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
    create_selector(4, 0x00000000, 0xFFFFF,            PRESENT | CPL_USER   | WRITABLE            | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
    create_selector(5, (uint32_t) &tss, sizeof(tss_t), PRESENT | CPL_KERNEL |            EXECABLE | TSS    , FLAG_BITS_32 | FLAG_GRANULARITY_BYTE);

    tss.ss0 = CPL_KERNEL_DATA;

    gdtd.size = (GDT_SIZE * sizeof(gdt_entry_t)) - 1;
    gdtd.offset = (uint32_t) gdt;

    __asm__ volatile("lgdt (%0)" :: "m" (gdtd));

    flush_segment_registers();
    flush_tss();

    return idt_init();
}

arch_initcall(gdt_init);
