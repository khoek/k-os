#ifndef KERNEL_SYNC_SEMAPHORE_H
#define KERNEL_SYNC_SEMAPHORE_H

#include "common/types.h"
#include "common/list.h"
#include "common/compiler.h"
#include "sync/spinlock.h"

typedef struct semaphore {
    spinlock_t lock;

    uint32_t count;
    list_head_t waiters;
} semaphore_t;

#define SEMAPHORE_WITH_COUNT(name, u) { .lock = SPINLOCK_UNLOCKED, .count = u, .waiters = LIST_HEAD((name).waiters) }

#define DEFINE_SEMAPHORE(name, u) semaphore_t name = SEMAPHORE_WITH_COUNT(name, u)

static inline void semaphore_init(semaphore_t *semaphore, uint32_t count) {
    *semaphore = (semaphore_t) SEMAPHORE_WITH_COUNT(*semaphore, count);
}

//Tries to acquire sem, sleeping if it fails, and when woken goes back to sleep
//so long as cond evaluates to true.
//WARNING: you must not try to acquire this semaphore when "cond" is evaluated.
//This will immediately deadlock.
//Returns true if we successfully decremented the semaphore.
#define try_semaphore_down_while_condition(sem, cond) ({ \
    uint32_t __semflags;                                          \
    bool __semgaveup = false;                                     \
    if(!_semaphore_down_begin(sem, &__semflags)) {                \
        do {                                                      \
            if(!(cond)) { __semgaveup = true; break; }            \
            _semaphore_down_yield(sem, &__semflags);              \
        } while(ACCESS_ONCE(current->flags) & THREAD_FLAG_INSEM); \
    }                                                             \
    if(__semgaveup) _semaphore_down_end_gaveup(sem, &__semflags); \
    else _semaphore_down_end_obtained(sem, &__semflags);          \
    !__semgaveup;                                                 \
  })

void semaphore_down(semaphore_t *sem);
void semaphore_up(semaphore_t *sem);

#include "sched/sched.h"

#endif
