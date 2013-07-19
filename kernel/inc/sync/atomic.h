#ifndef KERNEL_SYNC_ATOMIC_H
#define KERNEL_SYNC_ATOMIC_H

#include "lib/int.h"

typedef struct atomic {
    int32_t value;
} atomic_t;

int32_t atomic_read(atomic_t *a);
void atomic_set(atomic_t *a, int32_t v);

void atomic_inc(atomic_t *a);
void atomic_dec(atomic_t *a);

int32_t atomic_xchg(atomic_t *a, int32_t v);

void atomic_add(atomic_t *a, int32_t v);
int32_t atomic_add_and_return(atomic_t *a, int32_t v);

void atomic_sub(atomic_t *a, int32_t v);
int32_t atomic_sub_and_return(atomic_t *a, int32_t v);

#endif
