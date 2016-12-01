#include "arch/idt.h"
#include "sync/spinlock.h"

void spin_lock_irq(volatile spinlock_t *lock) {
    irqdisable();
    spin_lock(lock);
}

void spin_unlock_irq(volatile spinlock_t *lock) {
    spin_unlock(lock);
    irqenable();
}

void spin_lock_irqsave(volatile spinlock_t *lock, uint32_t *flags) {
    irqsave(flags);
    spin_lock(lock);
}

void spin_unlock_irqstore(volatile spinlock_t *lock, uint32_t flags) {
    spin_unlock(lock);
    irqstore(flags);
}
