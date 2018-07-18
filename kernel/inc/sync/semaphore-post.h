#ifndef KERNEL_SYNC_SEMAPHORE_POST_H
#define KERNEL_SYNC_SEMAPHORE_POST_H

void thread_sleep_prepare();
void sched_switch();

//sem lock held assumed
static void _semaphore_waitlist_add(semaphore_t *sem, thread_t *t) {
    spin_lock(&t->lock);
    list_add(&t->sleep_list, &sem->waiters);
    BUG_ON(t->flags & THREAD_FLAG_INSEM);
    t->flags |= THREAD_FLAG_INSEM;
    spin_unlock(&t->lock);
}

//sem lock held assumed
static void _semaphore_waitlist_rm(semaphore_t *sem, thread_t *t) {
    spin_lock(&t->lock);
    list_rm(&t->sleep_list);
    BUG_ON(!(t->flags & THREAD_FLAG_INSEM));
    t->flags &= ~THREAD_FLAG_INSEM;
    spin_unlock(&t->lock);
}

//exits with sem lock held
//returns whether true if we skipped the waitqueue
static bool _semaphore_down_begin(semaphore_t *sem, uint32_t *flags) {
    spin_lock_irqsave(&sem->lock, flags);

    if(!sem->count) {
        _semaphore_waitlist_add(sem, current);
    }

    return sem->count;
}

//enters with sem lock held, exits with released
static void _semaphore_down_end_obtained(semaphore_t *sem, uint32_t *flags) {
    BUG_ON(!sem->count);

    //If THREAD_FLAG_INSEM has been cleared, we have been removed from the sem
    //waiters list, so no need to do that here.
    sem->count--;
    spin_unlock_irqstore(&sem->lock, *flags);
}

//enters with sem lock held, exits with released
static void _semaphore_down_end_gaveup(semaphore_t *sem, uint32_t *flags) {
    _semaphore_waitlist_rm(sem, current);
    spin_unlock_irqstore(&sem->lock, *flags);
}

//sem lock held assumed
static void _semaphore_down_yield(semaphore_t *sem, uint32_t *flags) {
    BUG_ON(!(current->flags & THREAD_FLAG_INSEM));

    thread_sleep_prepare();

    spin_unlock_irqstore(&sem->lock, *flags);
    sched_switch();
    spin_lock_irqsave(&sem->lock, flags);
}

#endif
