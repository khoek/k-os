#ifndef KERNEL_COMMON_MMIO_H
#define KERNEL_COMMON_MMIO_H

static inline uint32_t readl(uint32_t offset, const volatile uint32_t *addr) {
    return *((volatile uint32_t *) (((uint32_t) addr) + offset));
}

static inline uint16_t readw(uint32_t offset, const volatile uint16_t *addr) {
    return *((const volatile uint16_t *) (((uint32_t) addr) + offset));
}

static inline uint8_t readb(uint32_t offset, const volatile uint8_t *addr) {
    return *((const volatile uint8_t *) (((uint32_t) addr) + offset));
}

static inline void writel(uint32_t val, uint32_t offset, volatile void *addr) {
    *((volatile uint32_t *) (((uint32_t) addr) + offset)) = val;
}

static inline void writew(uint16_t val, uint32_t offset, volatile void *addr) {
    *((volatile uint16_t *) (((uint32_t) addr) + offset)) = val;
}

static inline void writeb(uint8_t val, uint32_t offset, volatile void *addr) {
    *((volatile uint8_t *) (((uint32_t) addr) + offset)) = val;
}

#endif
