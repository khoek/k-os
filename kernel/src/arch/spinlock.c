#include "common/types.h"
#include "common/compiler.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "arch/idt.h"
#include "arch/proc.h"
#include "arch/cpu.h"
#include "time/clock.h"

#include "atomic_ops.h"

DEFINE_PER_CPU(uint32_t, locks_held);
DEFINE_PER_CPU(list_head_t, lock_list);

#define TICKET_SHIFT 16

bool _spin_trylock(volatile spinlock_t *lock) {
    check_irqs_disabled();
    BUG_ON(!lock);

    spinlock_arch_t old, new;

    old.tickets = ACCESS_ONCE(lock->arch.tickets);
    if (old.tickets.head != old.tickets.tail) {
        return 0;
    }
    new.raw = old.raw + (1 << TICKET_SHIFT);

    barrier();
    /* cmpxchg is a full barrier, so nothing can move before it */
    uint32_t res = cmpxchg(&lock->arch.raw, old.raw, new.raw);
    barrier();

    bool ret = res == old.raw;

    if(ret && percpu_up) {
        if(lock->arch.holder != 0xFFFF){
            panicf("spinlock lock violation %X vs %X", lock->arch.holder, get_percpu(this_proc)->num);
        }
        lock->arch.holder = get_percpu(this_proc)->num;
        list_add(&((spinlock_t *) lock)->list, &get_percpu(lock_list));
        get_percpu(locks_held)++;
    }

    return ret;
}

void _spin_lock(volatile spinlock_t *lock) {
    check_irqs_disabled();
    BUG_ON(!lock);

    register ticket_pair_t local = {.tail = 1};

    local = xchg_op(add, &lock->arch.tickets, local);
    if(likely(local.head == local.tail)) {
        goto lock_out;
    }

    while(true) {
        BUG_ON(!lock);
        if(ACCESS_ONCE(lock->arch.tickets.head) == local.tail) {
            goto lock_out;
        }
        relax();
    }

lock_out:
    barrier();

    if(percpu_up) {
        if(lock->arch.holder != 0xFFFF){
            panicf("spinlock lock violation %X vs %X", lock->arch.holder, get_percpu(this_proc)->num);
        }
        lock->arch.holder = get_percpu(this_proc)->num;
        list_add(&((spinlock_t *) lock)->list, &get_percpu(lock_list));
        get_percpu(locks_held)++;
    }
}

void _spin_unlock(volatile spinlock_t *lock) {
    check_irqs_disabled();
    BUG_ON(!lock);

    if(percpu_up) {
        if(lock->arch.holder == 0xFFFF){
            panicf("spinlock unlock violation %X vs %X", lock->arch.holder, get_percpu(this_proc)->num);
        }
        lock->arch.holder = 0xFFFF;
        list_rm(&((spinlock_t *) lock)->list);
        get_percpu(locks_held)--;
    }

    barrier();
    add(&lock->arch.tickets.head, 1);
    barrier();
}
