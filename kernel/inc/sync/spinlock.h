#ifndef KERNEL_SYNC_SPINLOCK_H
#define KERNEL_SYNC_SPINLOCK_H

typedef struct spinlock spinlock_t;

#include <stddef.h>

#include "lib/int.h"
#include "arch/spinlock.h"

struct spinlock {
    spinlock_arch_t arch;

#ifdef DEBUG_SPINLOCKS
    void *holder;
#endif
};

#ifdef DEBUG_SPINLOCKS
#define SPINLOCK_UNLOCKED {SPINLOCK_ARCH_UNLOCKED, NULL}
#define SPINLOCK_LOCKED   {SPINLOCK_ARCH_LOCKED,   NULL}
#else
#define SPINLOCK_UNLOCKED {SPINLOCK_ARCH_UNLOCKED}
#define SPINLOCK_LOCKED   {SPINLOCK_ARCH_LOCKED}
#endif

#define DECLARE_SPINLOCK(name) extern spinlock_t name
#define DEFINE_SPINLOCK(name) spinlock_t name = SPINLOCK_UNLOCKED

static inline void spinlock_init(spinlock_t *spinlock) {
    *spinlock = (spinlock_t) SPINLOCK_UNLOCKED;
}

void spin_lock_irq(volatile spinlock_t *lock);
void spin_unlock_irq(volatile spinlock_t *lock);

void spin_lock_irqsave(volatile spinlock_t *lock, uint32_t *flags);
void spin_unlock_irqstore(volatile spinlock_t *lock, uint32_t flags);

#endif
