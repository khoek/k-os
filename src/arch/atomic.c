#include "compiler.h"
#include "atomic.h"

#include "atomic_ops.h"

int32_t atomic_read(atomic_t *a) {
    return ACCESS_ONCE(a->value);
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

int32_t atomic_add_and_return(atomic_t *a, int32_t v) {
    return v + xchg_op(add, &a->value, v);
}

void atomic_sub(atomic_t *a, int32_t v) {
    register int val = v;
    __asm__ volatile("lock sub %1, %0" : "=m" (a->value), "=r" (val) : "1" (v));
}
