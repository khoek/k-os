#include "common/types.h"
#include "common/list.h"
#include "arch/proc.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "sched/sched.h"
#include "log/log.h"

void semaphore_down(semaphore_t *sem) {
    BUG_ON(!try_semaphore_down_while_condition(sem, true));

    //Uncomment this if we ever change try_semaphore_down_while_condition() so
    //that the call above is less efficient.

    // uint32_t flags;
    // if(!_semaphore_down_begin(sem, &flags)) {
    //     do {
    //         _semaphore_down_yield(sem, &flags);
    //     } while(ACCESS_ONCE(current->flags) & THREAD_FLAG_INSEM);
    // }
    // _semaphore_down_end_obtained(sem, &flags);
}

void semaphore_up(semaphore_t *sem) {
    uint32_t flags;
    spin_lock_irqsave(&sem->lock, &flags);

    sem->count++;
    if(!list_empty(&sem->waiters)) {
        thread_t *waiting = list_first(&sem->waiters, thread_t, sleep_list);
        _semaphore_waitlist_rm(sem, waiting);

        thread_wake(waiting);
    }

    spin_unlock_irqstore(&sem->lock, flags);
}
