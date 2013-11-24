#include <stddef.h>

#include "lib/string.h"
#include "init/initcall.h"
#include "bug/panic.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/registers.h"
#include "mm/mm.h"
#include "sched/proc.h"
#include "video/log.h"

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

DEFINE_PER_CPU(gdt_entry_t, gdt[GDT_SIZE]);
DEFINE_PER_CPU(tss_t, tss);

static uint8_t kernel_stack[0x1000];

#define FLAG_PRESENT (1 << 7)

#define TYPE_TSS ((1 << 0) | FLAG_PRESENT)
#define TYPE_SEG ((1 << 4) | FLAG_PRESENT)

#define FLAG_AVAILABLE        (1 << 4)
#define BITS_32          (1 << 6)
#define FLAG_GRANULARITY_BYTE (0 << 7)
#define FLAG_GRANULARITY_PAGE (1 << 7)

#define PERM_W (1 << 1)
#define PERM_X (1 << 3)
#define PERM_WX (PERM_W | PERM_X)

static inline void set_selector(gdt_entry_t *gdt, uint16_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    index /= 0x8;

    gdt[index].limit_0_15 = limit & 0xFFFF;
    gdt[index].limit_16_19_and_flags = flags | ((limit >> 16) & 0xF);
    gdt[index].base_0_15 = base & 0xFFFF;
    gdt[index].base_16_23 = (base >> 16) & 0xFF;
    gdt[index].base_24_31 = base >> 24;
    gdt[index].access = access;
}

void tss_set_stack(uint32_t sp) {
    tss_t *tss = &get_percpu_unsafe(tss);

    tss->esp0 = sp;
}

void gdt_set_tls(uint32_t tls_start) {
    gdt_entry_t *gdt = get_percpu(gdt);
    set_selector(gdt, SEL_USER_TLS  , tls_start, 0xFFFFF, TYPE_SEG | CPL_USER | PERM_W , BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
}

extern uint32_t percpu_data_start;
extern uint32_t percpu_data_end;

void gdt_init(processor_t *proc) {
    gdt_entry_t *gdt = get_percpu_raw(proc->percpu_data, gdt);
    tss_t *tss = &(get_percpu_raw(proc->percpu_data, tss));

    set_selector(gdt, SEL_KRNL_CODE, 0x00000000, 0xFFFFF, TYPE_SEG | CPL_KRNL | PERM_WX, BITS_32 | FLAG_GRANULARITY_PAGE);
    set_selector(gdt, SEL_KRNL_DATA, 0x00000000, 0xFFFFF, TYPE_SEG | CPL_KRNL | PERM_W , BITS_32 | FLAG_GRANULARITY_PAGE);
    set_selector(gdt, SEL_USER_CODE, 0x00000000, 0xFFFFF, TYPE_SEG | CPL_USER | PERM_W , BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
    set_selector(gdt, SEL_USER_DATA, 0x00000000, 0xFFFFF, TYPE_SEG | CPL_USER | PERM_W , BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);
    set_selector(gdt, SEL_USER_TLS , 0x00000000, 0xFFFFF, TYPE_SEG | CPL_USER | PERM_W , BITS_32 | FLAG_GRANULARITY_PAGE | FLAG_AVAILABLE);

    set_selector(gdt, SEL_KRNL_PCPU, (uint32_t) &proc->percpu_data, 0xFFFFF, TYPE_SEG | CPL_KRNL | PERM_W, BITS_32 | FLAG_GRANULARITY_BYTE);

    set_selector(gdt, SEL_TSS, (uint32_t) tss, sizeof(tss_t), TYPE_TSS | CPL_KRNL | PERM_X, BITS_32 | FLAG_GRANULARITY_BYTE);

    tss->ss0 = SEL_KRNL_DATA;
    tss->esp0 = ((uint32_t) kernel_stack) + sizeof(kernel_stack) - 1;

    gdtd_t gdtd;
    gdtd.size = (GDT_SIZE * sizeof(gdt_entry_t)) - 1;
    gdtd.offset = (uint32_t) gdt;

    __asm__ volatile("lgdt (%0)" :: "a" (&gdtd));

    flush_segment_registers();

    __asm__ volatile("mov %0, %%gs" :: "a" (SEL_KRNL_PCPU));
    __asm__ volatile("ltr %%ax" :: "a" (SEL_TSS | SPL_USER));
}
