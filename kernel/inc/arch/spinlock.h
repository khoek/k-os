#ifndef KERNEL_ARCH_SPINLOCK_H
#define KERNEL_ARCH_SPINLOCK_H

#include "common/types.h"
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

typedef struct spinlock_arch  {
    union {
        ticket_pair_t tickets;
        uint32_t raw;
    };
    uint32_t holder;
} PACKED spinlock_arch_t;

#define SPINLOCK_ARCH_LOCKED   {.tickets.head = 0, .tickets.tail = 1, .holder = 0xFFFF}
#define SPINLOCK_ARCH_UNLOCKED {.tickets.head = 0, .tickets.tail = 0, .holder = 0xFFFF}

#define spin_lock(x) \
    {                                \
        check_irqs_disabled();       \
        barrier();                   \
        _spin_lock(x);               \
        barrier();                   \
    }

#define spin_trylock(x) \
    ({                               \
        check_irqs_disabled();       \
        barrier();                   \
        bool ret = _spin_trylock(x); \
        barrier();                   \
        ret;                         \
    })

#define spin_unlock(x) \
    {                          \
        barrier();             \
        check_irqs_disabled(); \
        barrier();             \
        _spin_unlock(x);       \
    }

void _spin_lock(volatile spinlock_t *lock);
void _spin_unlock(volatile spinlock_t *lock);
bool _spin_trylock(volatile spinlock_t *lock);

#endif
