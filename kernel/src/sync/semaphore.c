#include "common/types.h"
#include "common/list.h"
#include "arch/proc.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "sched/sched.h"
#include "log/log.h"

void semaphore_down(semaphore_t *lock) {
    uint32_t flags;
    spin_lock_irqsave(&lock->lock, &flags);

    if(lock->count) {
        lock->count--;

        spin_unlock_irqstore(&lock->lock, flags);
    } else {
        thread_sleep_prepare();

        list_add(&current->state_list, &lock->waiters);

        spin_unlock_irqstore(&lock->lock, flags);

        sched_switch();
    }
}

void semaphore_up(semaphore_t *lock) {
    uint32_t flags;
    spin_lock_irqsave(&lock->lock, &flags);
    if(list_empty(&lock->waiters)) {
        lock->count++;
    } else {
        thread_t *waiting = list_first(&lock->waiters, thread_t, state_list);
        list_rm(&waiting->state_list);

        thread_wake(waiting);
    }

    spin_unlock_irqstore(&lock->lock, flags);
}
