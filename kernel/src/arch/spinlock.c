#include "lib/int.h"
#include "common/compiler.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "bug/panic.h"
#include "arch/registers.h"
#include "time/clock.h"

#include "atomic_ops.h"

#define LOCK_ATTEMPTS (1 << 15)
#define DEADLOCK_THRESHOLD 1000

#ifdef CONFIG_DEBUG_SPINLOCKS
static void check_safe() {
    if(get_eflags() & EFLAGS_IF) panic("spinlock usage in interruptible context!");
}
#endif

void _spin_lock(volatile spinlock_t *lock) {
#ifdef CONFIG_DEBUG_SPINLOCKS
    check_safe();

    uint64_t then = uptime();
#endif

    register ticket_pair_t local = {.tail = 1};
    local = xchg_op(add, &lock->arch, local);
    if(likely(local.head == local.tail))
        goto lock_out;

    while(true) {
        uint32_t attempts = LOCK_ATTEMPTS;
        do {
            if(ACCESS_ONCE(lock->arch.head) == local.tail)
                goto lock_out;
            relax();

#ifdef CONFIG_DEBUG_SPINLOCKS
            if(uptime() - then > DEADLOCK_THRESHOLD) {
                panicf("deadlock detected! state: (%X, %X), offender: 0x%X", ACCESS_ONCE(lock->arch.head), ACCESS_ONCE(lock->arch.tail), lock->holder);
            }
#endif
        } while(--attempts);

        //TODO consider blocking
    }

#ifdef CONFIG_DEBUG_SPINLOCKS
    lock->holder = __builtin_return_address(0);
#endif

lock_out:
    barrier();
}

void _spin_unlock(volatile spinlock_t *lock) {
#ifdef CONFIG_DEBUG_SPINLOCKS
    check_safe();

    lock->holder = NULL;
#endif

    add(&lock->arch.head, 1);
}

void spin_lock_irq(volatile spinlock_t *lock) {
    cli(); //TODO SMP fix (disable locally)
    spin_lock(lock);
}

void spin_unlock_irq(volatile spinlock_t *lock) {
    spin_unlock(lock);
    sti(); //TODO SMP fix (disable locally)
}

void spin_lock_irqsave(volatile spinlock_t *lock, uint32_t *flags) {
    *flags = get_eflags() & EFLAGS_IF;

    cli(); //TODO SMP fix (disable locally)
    spin_lock(lock);
}

void spin_unlock_irqstore(volatile spinlock_t *lock, uint32_t flags) {
    spin_unlock(lock);

    if(flags & EFLAGS_IF) sti(); //TODO SMP fix (disable locally)

    //Alternatively:
    //set_eflags((get_eflags() & ~EFLAGS_IF) | flags);
}
