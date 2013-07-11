#ifndef KERNEL_ARCH_MATH_H
#define KERNEL_ARCH_MATH_H

#include "lib/int.h"
#include "common/math.h"

static inline uint32_t fls32(uint32_t x) {
    uint32_t r;
    asm("bsrl %1,%0"
        : "=r" (r)
        : "rm" (x), "" (-1));
    return r + 1;
}

static inline uint32_t fls64(uint64_t x) {
    uint32_t bitpos = -1;
    asm("bsrq %1,%q0"
        : "+r" (bitpos)
        : "rm" (x));
    return bitpos + 1;
}

static inline uint32_t log2_32(uint32_t n) {
        return fls32(n) - 1;
}

static inline uint32_t log2_64(uint64_t n) {
        return fls64(n) - 1;
}

#endif
