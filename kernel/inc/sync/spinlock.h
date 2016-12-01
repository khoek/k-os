#ifndef KERNEL_SYNC_SPINLOCK_H
#define KERNEL_SYNC_SPINLOCK_H

typedef struct spinlock spinlock_t;

#include "common/types.h"
#include "common/list.h"
#include "arch/spinlock.h"

struct spinlock {
    spinlock_arch_t arch;
    list_head_t list;
};

#define SPINLOCK_UNLOCKED {.arch = SPINLOCK_ARCH_UNLOCKED}
#define SPINLOCK_LOCKED   {.arch = SPINLOCK_ARCH_LOCKED}

#define DECLARE_SPINLOCK(name) extern spinlock_t name
#define DEFINE_SPINLOCK(name) spinlock_t name = SPINLOCK_UNLOCKED

static inline void spinlock_init(spinlock_t *spinlock) {
    *spinlock = (spinlock_t) SPINLOCK_UNLOCKED;
    list_init(&spinlock->list);
}

void spin_lock_irq(volatile spinlock_t *lock);
void spin_unlock_irq(volatile spinlock_t *lock);

void spin_lock_irqsave(volatile spinlock_t *lock, uint32_t *flags);
void spin_unlock_irqstore(volatile spinlock_t *lock, uint32_t flags);

#endif
