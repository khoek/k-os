#include "common/compiler.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "arch/registers.h"
#include "video/log.h"

#include "atomic_ops.h"

void spin_lock(spinlock_t *lock) {
    ticket_pair_t local = {.tail = 1};
    local = xchg_op(add, &lock->tickets, local);

    while(local.head != local.tail) {
        local.head = ACCESS_ONCE(lock->tickets.head);
    }
}

void spin_unlock(spinlock_t *lock) {
    add(&lock->tickets.head, 1);
}

void spin_lock_irq(spinlock_t *lock) {
    cli(); //TODO SMP fix (disable locally)
    spin_lock(lock);
}

void spin_unlock_irq(spinlock_t *lock) {
    spin_unlock(lock);
    sti(); //TODO SMP fix (disable locally)
}

void spin_lock_irqsave(spinlock_t *lock, uint32_t *flags) {
    *flags = get_eflags() & 0x200 ? 1 : 0;

    cli(); //TODO SMP fix (disable locally)
    spin_lock(lock);
}

void spin_unlock_irqstore(spinlock_t *lock, uint32_t flags) {
    spin_unlock(lock);

    if(flags) sti(); //TODO SMP fix (disable locally)
}
