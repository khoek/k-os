#include "compiler.h"
#include "spinlock.h"
#include "log.h"

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
