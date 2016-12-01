#include "common/list.h"
#include "sync/spinlock.h"
#include "arch/cpu.h"
#include "time/timer.h"
#include "time/clock.h"
#include "mm/cache.h"

typedef struct timer {
    list_head_t list;

    uint32_t delta;
    timer_callback_t callback;
    void *data;
} timer_t;

static cache_t *timer_cache;

static DEFINE_LIST(active_timers);

static DEFINE_SPINLOCK(timer_lock);

void timer_create(uint32_t millis, timer_callback_t callback, void *data) {
    timer_t *new = cache_alloc(timer_cache);
    new->delta = millis;
    new->callback = callback;
    new->data = data;

    uint32_t flags;
    spin_lock_irqsave(&timer_lock, &flags);

    if(list_empty(&active_timers)) {
        list_add(&new->list, &active_timers);
    } else {
        timer_t *timer;
        LIST_FOR_EACH_ENTRY(timer, &active_timers, list) {
            if(!new->delta || new->delta < timer->delta) {
                timer->delta -= new->delta;
                list_add_before(&new->list, &timer->list);
                timer = NULL;
                break;
            }

            new->delta -= timer->delta;
        }

        if(timer != NULL) {
            list_add_before(&new->list, active_timers.prev);
        }
    }

    spin_unlock_irqstore(&timer_lock, flags);
}

static uint32_t decrement_timer(timer_t *timer, uint32_t ms) {
    uint32_t diff = MIN(timer->delta, ms);
    timer->delta -= diff;
    return ms - diff;
}

static void time_tick(clock_event_source_t *source) {
    uint32_t flags;
    spin_lock_irqsave(&timer_lock, &flags);

    uint32_t left = 1; //FIXME calc from source->freq
    timer_t *timer;
    LIST_FOR_EACH_ENTRY(timer, &active_timers, list) {
        left = decrement_timer(timer, left);
        if(timer->delta) {
            break;
        }

        timer->callback(timer->data);

        cache_free(timer_cache, timer);
    }

    while(!list_empty(&active_timers)) {
        timer = list_first(&active_timers, timer_t, list);
        if(timer->delta) {
            break;
        }
        list_rm(&timer->list);
    }

    spin_unlock_irqstore(&timer_lock, flags);
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
