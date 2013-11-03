#include <stddef.h>

#include "lib/string.h"
#include "init/initcall.h"
#include "bug/panic.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/registers.h"
#include "video/log.h"

#define GDT_SIZE 7

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

gdtd_t gdtd;
static gdt_entry_t gdt[GDT_SIZE];
static tss_t tss;

static uint8_t kernel_stack[0x1000];

#define PRESENT     (1 << 7)
#define TSS         (1 << 0)
#define SEGMENT     (1 << 4)
#define EXECABLE    (1 << 3)
#define WRITABLE    (1 << 1)

#define FLAG_AVAILABLE        (1 << 4)
#define FLAG_BITS_32          (1 << 6)
#define FLAG_GRANULARITY_BYTE (0 << 7)
#define FLAG_GRANULARITY_PAGE (1 << 7)

static void set_selector(uint16_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[index].limit_0_15 = limit & 0xFFFF;
    gdt[index].limit_16_19_and_flags = flags | ((limit >> 16) & 0xF);
    gdt[index].base_0_15  = base & 0xFFFF;
    gdt[index].base_16_23 = (base >> 16) & 0x00FF;
    gdt[index].base_24_31 = base >> 24;
    gdt[index].access     = access;
}

void tss_set_stack(uint32_t sp) {
    tss.esp0 = sp;
}

void gdt_set_tls(uint32_t tls_start) {
    set_selector(5, tls_start , 0xFFFFF,            PRESENT | CPL_USER   | WRITABLE            | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
}

static INITCALL gdt_init() {
    set_selector(0, 0x00000000, 0x00000, 0, 0);
    set_selector(1, 0x00000000, 0xFFFFF,            PRESENT | CPL_KERNEL | WRITABLE | EXECABLE | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE);
    set_selector(2, 0x00000000, 0xFFFFF,            PRESENT | CPL_KERNEL | WRITABLE            | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE);
    set_selector(3, 0x00000000, 0xFFFFF,            PRESENT | CPL_USER   | WRITABLE | EXECABLE | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
    set_selector(4, 0x00000000, 0xFFFFF,            PRESENT | CPL_USER   | WRITABLE            | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
    set_selector(5, 0x00000000, 0xFFFFF,            PRESENT | CPL_USER   | WRITABLE            | SEGMENT, FLAG_BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
    set_selector(6, (uint32_t) &tss, sizeof(tss_t), PRESENT | CPL_KERNEL |            EXECABLE | TSS    , FLAG_BITS_32 | FLAG_GRANULARITY_BYTE);

    tss.ss0 = SEL_KERNEL_DATA;

    tss_set_stack(((uint32_t) kernel_stack) + sizeof(kernel_stack) - 1);

    gdtd.size = (GDT_SIZE * sizeof(gdt_entry_t)) - 1;
    gdtd.offset = (uint32_t) gdt;

    __asm__ volatile("lgdt (%0)" :: "m" (gdtd));

    flush_segment_registers();

    __asm__ volatile("ltr %%ax" :: "a" (SEL_TSS | SPL_USER));

    logf("gdt - gdt and tss are active!");

    return idt_setup();
}

arch_initcall(gdt_init);
