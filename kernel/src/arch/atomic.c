#include <stdbool.h>

#include "common/compiler.h"
#include "sync/atomic.h"

#include "atomic_ops.h"

int32_t atomic_read(atomic_t *a) {
    return ACCESS_ONCE(a->value);
}

void atomic_set(atomic_t *a, int32_t v) {
    a->value = v;
}

void atomic_inc(atomic_t *a) {
    asm volatile("lock incl %0" : "=m" (a->value));
}

bool atomic_inc_and_test(atomic_t *a) {
    uint8_t c;
    asm volatile("lock incl %0; sete %1" : "+m" (a->value), "=qm" (c) : : "memory");
    return c != 0;
}

void atomic_dec(atomic_t *a) {
    asm volatile("lock decl %0" : "=m" (a->value));
}

bool atomic_dec_and_test(atomic_t *a) {
    uint8_t c;
    asm volatile("lock decl %0; sete %1" : "+m" (a->value), "=qm" (c) : : "memory");
    return c != 0;
}

int32_t atomic_xchg(atomic_t *a, int32_t v) {
    register int val = v;
    asm volatile("lock xchg %1, %0" : "=m" (a->value), "=r" (val) : "1" (v));
    return val;
}

void atomic_add(atomic_t *a, int32_t v) {
    register int val = v;
    asm volatile("lock add %1, %0" : "=m" (a->value), "=r" (val) : "1" (v));
}

int32_t atomic_add_and_return(atomic_t *a, int32_t v) {
    return v + xchg_op(add, &a->value, v);
}

void atomic_sub(atomic_t *a, int32_t v) {
    register int val = v;
    asm volatile("lock sub %1, %0" : "=m" (a->value), "=r" (val) : "1" (v));
}
