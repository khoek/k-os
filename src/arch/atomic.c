#include "atomic.h"

int32_t atomic_read(atomic_t *a) {
    return (*((volatile int *) &(a->value)));
}

void atomic_set(atomic_t *a, int32_t v) {
    a->value = v;
}

void atomic_inc(atomic_t *a) {
    __asm__ volatile("lock incl %0" : "=m" (a->value));
}

void atomic_dec(atomic_t *a) {
    __asm__ volatile("lock decl %0" : "=m" (a->value));
}

int32_t atomic_xchg(atomic_t *a, int32_t v) {
    register int val = v;
    __asm__ volatile("lock xchg %1, %0" : "=m" (a->value), "=r" (val) : "1" (v));
    return val;
}

void atomic_add(atomic_t *a, int32_t v) {
    register int val = v;
    __asm__ volatile("lock add %1, %0" : "=m" (a->value), "=r" (val) : "1" (v));
}

#define __X86_CASE_B    1
#define __X86_CASE_W    2
#define __X86_CASE_L    4

#define xchg_op(op, ptr, arg)                                                                                   \
    ({                                                                                                          \
        __typeof__ (*(ptr)) __ret = (arg);                                                                      \
        switch (sizeof(*(ptr))) {                                                                               \
            case __X86_CASE_B:                                                                                  \
                asm volatile ("lock; x" #op "b %b0, %1\n" : "+q" (__ret), "+m" (*(ptr)) : : "memory", "cc");    \
                break;                                                                                          \
            case __X86_CASE_W:                                                                                  \
                asm volatile ("lock; x" #op "w %w0, %1\n" : "+r" (__ret), "+m" (*(ptr)) : : "memory", "cc");    \
                break;                                                                                          \
            case __X86_CASE_L:                                                                                  \
                asm volatile ("lock; x" #op "l %0, %1\n" : "+r" (__ret), "+m" (*(ptr)) : : "memory", "cc");     \
                break;                                                                                          \
        }                                                                                                       \
        __ret;                                                                                                  \
    })

int32_t atomic_add_and_return(atomic_t *a, int32_t v) {
    return v + xchg_op(add, &a->value, v);
}

void atomic_sub(atomic_t *a, int32_t v) {
    register int val = v;
    __asm__ volatile("lock sub %1, %0" : "=m" (a->value), "=r" (val) : "1" (v));
}
