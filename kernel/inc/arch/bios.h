#ifndef KERNEL_ARCH_BIOS_H
#define KERNEL_ARCH_BIOS_H

#define BIOS_VRAM     0xB8000
#define BIOS_BDA      0x400

#define BDA_EBDA_ADDR 0x0E
#define BDA_VRAM_PORT 0x63
#define BDA_RESET_VEC 0x67

#define BIOS_SPACE_START 0x8000
#define BIOS_SPACE_END (0x7FFFF + 1)

#define VRAM_START 0xE0000
#define VRAM_END   (0xFFFFF + 1)

#include "lib/int.h"
#include "common/math.h"

#define VRAM_PAGES (DIV_UP(VRAM_END - VRAM_START, PAGE_SIZE))

extern uint8_t *bios_bda_ptr;

static inline uint16_t bda_getw(uint32_t off) {
    return *((volatile uint16_t *) (bios_bda_ptr + off));
}

static inline void bda_putw(uint32_t off, uint16_t val) {
    *((volatile uint16_t *) (bios_bda_ptr + off)) = val;
}

static inline uint32_t bda_getl(uint32_t off) {
    return *((volatile uint32_t *) (bios_bda_ptr + off));
}

static inline void bda_putl(uint32_t off, uint32_t val) {
    *((volatile uint32_t *) (bios_bda_ptr + off)) = val;
}

#endif
