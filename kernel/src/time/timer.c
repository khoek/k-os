#include "common/list.h"
#include "atomic/spinlock.h"
#include "arch/registers.h"
#include "time/timer.h"
#include "time/clock.h"
#include "mm/cache.h"
#include "video/log.h"

typedef struct timer {
    list_head_t list;

    uint32_t delta;
    void (*callback)(void *);
    void *data;
} timer_t;

static cache_t *timer_cache;

static LIST_HEAD(active_timers);
static LIST_HEAD(dispatch_timers);

static SPINLOCK_INIT(active_lock);
static SPINLOCK_INIT(dispatch_lock);

void timer_create(uint32_t millis, void (*callback)(void *), void *data) {
    timer_t *new = cache_alloc(timer_cache);
    new->delta = millis;
    new->callback = callback;
    new->data = data;

    uint32_t flags;
    spin_lock_irqsave(&active_lock, &flags);

    if(list_empty(&active_timers)) {
        list_add(&new->list, &active_timers);
    } else {
        timer_t *timer;
        LIST_FOR_EACH_ENTRY(timer, &active_timers, list) {
            if(!new->delta || new->delta < timer->delta) {
                break;
            }

            new->delta -= timer->delta;
        }

        if(&timer->list != &active_timers) timer->delta -= new->delta;
        list_add_before(&new->list, &timer->list);
    }

    spin_unlock_irqstore(&active_lock, flags);
}

static void time_tick(clock_event_source_t *source) {
    if(list_empty(&active_timers)) return;

    uint32_t flags;
    spin_lock_irqsave(&active_lock, &flags);
    spin_lock_irq(&dispatch_lock);

    timer_t *timer = list_first(&active_timers, timer_t, list);
    timer->delta--; //FIXME calculate a value based on event freq
    while(!timer->delta) {
        list_move(&timer->list, &dispatch_timers);
        timer = list_first(&active_timers, timer_t, list);
    }

    spin_unlock(&active_lock);

    LIST_FOR_EACH_ENTRY(timer, &dispatch_timers, list) {
        timer->callback(timer->data);

        cache_free(timer_cache, timer);
    }

    list_init(&dispatch_timers);

    spin_unlock_irqstore(&dispatch_lock, flags);
}

static clock_event_listener_t clock_listener = {
    .handle = time_tick
};

static INITCALL timer_init() {
    timer_cache = cache_create(sizeof(timer_t));

    register_clock_event_listener(&clock_listener);

    return 0;
}

core_initcall(timer_init);
