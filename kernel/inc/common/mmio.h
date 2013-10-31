#ifndef KERNEL_COMMON_MMIO_H
#define KERNEL_COMMON_MMIO_H

static inline uint32_t readl(volatile void *addr, uint32_t offset) {
    return *((volatile uint32_t *) (((uint32_t) addr) + offset));
}

static inline uint16_t readw(volatile void *addr, uint32_t offset) {
    return *((volatile uint16_t *) (((uint32_t) addr) + offset));
}

static inline uint8_t readb(volatile void *addr, uint32_t offset) {
    return *((volatile uint8_t *) (((uint32_t) addr) + offset));
}

static inline void writel(volatile void *addr, uint32_t offset, uint32_t val) {
    *((volatile uint32_t *) (((uint32_t) addr) + offset)) = val;
}

static inline void writew(volatile void *addr, uint32_t offset, uint16_t val) {
    *((volatile uint16_t *) (((uint32_t) addr) + offset)) = val;
}

static inline void writeb(volatile void *addr, uint32_t offset, uint8_t val) {
    *((volatile uint8_t *) (((uint32_t) addr) + offset)) = val;
}

#endif
