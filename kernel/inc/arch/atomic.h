#ifndef KERNEL_ARCH_ATOMIC_H
#define KERNEL_ARCH_ATOMIC_H

#include "common/types.h"

static inline void set_bit(volatile uint32_t *addr, uint32_t nr) {
     asm volatile("lock; bts %1,%0" : "+m" (*(volatile long *) (addr)) : "Ir" (nr) : "memory");
}

static inline bool test_bit(volatile uint32_t *addr, uint32_t nr) {
    int bit;
    asm volatile("bt %2,%1\n\t"
                 "sbb %0,%0"   :
                 "=r" (bit) :
                  "m" (*(unsigned long *)addr), "Ir" (nr));
    return bit;
}

#endif
