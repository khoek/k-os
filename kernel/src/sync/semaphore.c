#include "common/types.h"
#include "common/list.h"
#include "arch/proc.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "sched/sched.h"
#include "log/log.h"

void semaphore_down(semaphore_t *sem) {
    uint32_t flags;
    spin_lock_irqsave(&sem->lock, &flags);

    if(!sem->count) {
        spin_lock(&current->lock);
        
        list_add(&current->sleep_list, &sem->waiters);
        current->flags |= THREAD_FLAG_INSEM;

        spin_unlock(&current->lock);
    }

    while(ACCESS_ONCE(current->flags) & THREAD_FLAG_INSEM) {
        thread_sleep_prepare();

        spin_unlock_irqstore(&sem->lock, flags);
        sched_switch();
        spin_lock_irqsave(&sem->lock, &flags);
    }

    BUG_ON(!sem->count);

    //If THREAD_FLAG_INSEM has been cleared, we have been removed from the sem
    //waiters list, so no need to do that here.

    sem->count--;
    spin_unlock_irqstore(&sem->lock, flags);
}

void semaphore_up(semaphore_t *sem) {
    uint32_t flags;
    spin_lock_irqsave(&sem->lock, &flags);

    sem->count++;
    if(!list_empty(&sem->waiters)) {
        thread_t *waiting = list_first(&sem->waiters, thread_t, sleep_list);
        list_rm(&waiting->sleep_list);

        spin_lock(&waiting->lock);
        BUG_ON(!(waiting->flags & THREAD_FLAG_INSEM));
        ACCESS_ONCE(waiting->flags) &= ~THREAD_FLAG_INSEM;
        spin_unlock(&waiting->lock);

        thread_wake(waiting);
    }

    spin_unlock_irqstore(&sem->lock, flags);
}
