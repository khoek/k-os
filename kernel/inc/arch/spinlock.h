#ifndef KERNEL_ARCH_SPINLOCK_H
#define KERNEL_ARCH_SPINLOCK_H

#include "lib/int.h"
#include "common/compiler.h"
#include "common/asm.h"
#include "sync/spinlock.h"

typedef uint16_t ticket_t;

//Must be strictly in this order, or else head will overflow into tail
//This ordering will arrange a DWORD in memory like (0xTTTTHHHH)
typedef struct ticket_pair {
    ticket_t head;
    ticket_t tail;
} PACKED ticket_pair_t;

typedef ticket_pair_t spinlock_arch_t;

#define SPINLOCK_ARCH_LOCKED   {.head = 0, .tail = 1}
#define SPINLOCK_ARCH_UNLOCKED {.head = 0, .tail = 0}

#define spin_lock(x) \
    {                       \
        _spin_lock(x);      \
        barrier();          \
    }

#define spin_unlock(x) \
    {                       \
        barrier();          \
        _spin_unlock(x);    \
    }

void _spin_lock(volatile spinlock_t *lock);
void _spin_unlock(volatile spinlock_t *lock);

#endif
