#ifndef KERNEL_SYNC_SEMAPHORE_H
#define KERNEL_SYNC_SEMAPHORE_H

#include "lib/int.h"
#include "common/list.h"
#include "common/compiler.h"
#include "sync/spinlock.h"
#include "sched/task.h"

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

void semaphore_down(semaphore_t *lock);
void semaphore_up(semaphore_t *lock);

#endif
