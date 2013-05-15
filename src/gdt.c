#include <stddef.h>
#include "gdt.h"
#include "string.h"
#include "common.h"
#include "console.h"
#include "panic.h"

#define GDT_SIZE 5

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

#define ACCESS_BASE (1 << 4)
#define PRESENT     (1 << 7)
#define RING_KERNEL (0 << 5)
#define RING_USER   (3 << 5)
#define TYPE_DATA   (0 << 3)
#define TYPE_CODE   (1 << 3)
#define PERM_R      (0 << 1)
#define PERM_RW     (1 << 1)

#define FLAG_BITS_16 (0 << 6)
#define FLAG_BITS_32 (1 << 6)
#define FLAG_GRANULARITY_BYTE (0 << 7)
#define FLAG_GRANULARITY_PAGE (1 << 7)

void setSelector(uint16_t index, uint32_t base, uint32_t limit, uint8_t type) {
    gdt[index].limit_0_15 = limit & 0xFFFF;
    gdt[index].limit_16_19_and_flags |= FLAG_BITS_32 | FLAG_GRANULARITY_PAGE | ((limit >> 16) & 0xF);
    gdt[index].base_0_15  = base & 0xFFFF;
    gdt[index].base_16_23 = (base >> 16) & 0x00FF;
    gdt[index].base_24_31 = base >> 24;
    gdt[index].access     = (type ? ACCESS_BASE : 0) | type;
}

void reload_segment_registers();

void gdt_init() {
    setSelector(0, 0x00000000, 0x00000, 0);
    setSelector(1, 0x00000000, 0xFFFFF, RING_KERNEL | TYPE_CODE | PERM_RW | PRESENT);
    setSelector(2, 0x00000000, 0xFFFFF, RING_KERNEL | TYPE_DATA | PERM_RW | PRESENT);
    setSelector(3, 0x00000000, 0xFFFFF, RING_USER   | TYPE_CODE | PERM_RW | PRESENT);
    setSelector(4, 0x00000000, 0xFFFFF, RING_USER   | TYPE_DATA | PERM_RW | PRESENT);

    gdtd.size = (GDT_SIZE * sizeof(gdt_entry_t)) - 1;
    gdtd.offset = (uint32_t) gdt;

    __asm__ volatile("lgdt (%0)" :: "m" (gdtd));

    reload_segment_registers();
}
