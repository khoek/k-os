#include "common/list.h"
#include "init/initcall.h"
#include "common/asm.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "time/clock.h"
#include "arch/idt.h"
#include "log/log.h"
#include "sync/spinlock.h"

static DEFINE_SPINLOCK(clock_lock);
static DEFINE_SPINLOCK(event_lock);

static DEFINE_LIST(clocks);
static DEFINE_LIST(clock_event_sources);
static DEFINE_LIST(clock_event_listeners);

static clock_t *active;
static clock_event_source_t *active_event_source;

static void handle_clock_event(clock_event_source_t *clock_event_source) {
    check_irqs_disabled();

    spin_lock(&event_lock);

    clock_event_listener_t *listener;
    LIST_FOR_EACH_ENTRY(listener, &clock_event_listeners, list) {
        listener->handle(clock_event_source);
    }

    spin_unlock(&event_lock);
}

static void handle_clock_nop(clock_event_source_t *clock_event_source) {
    check_irqs_disabled();
}

void register_clock(clock_t *clock) {
    uint32_t flags;
    spin_lock_irqsave(&clock_lock, &flags);

    if(!active || active->rating < clock->rating) {
        active = clock;
    }

    list_add(&clock->list, &clocks);

    spin_unlock_irqstore(&clock_lock, flags);
}

void register_clock_event_source(clock_event_source_t *clock_event_source) {
    uint32_t flags;
    spin_lock_irqsave(&event_lock, &flags);

    if(!active_event_source || active_event_source->rating < clock_event_source->rating) {
        active_event_source = clock_event_source;
    }
    clock_event_source->event = handle_clock_nop;

    list_add(&clock_event_source->list, &clock_event_sources);

    spin_unlock_irqstore(&event_lock, flags);
}

void register_clock_event_listener(clock_event_listener_t *clock_event_listener) {
    uint32_t flags;
    spin_lock_irqsave(&event_lock, &flags);

    list_add(&clock_event_listener->list, &clock_event_listeners);

    spin_unlock_irqstore(&event_lock, flags);
}

static uint64_t _unsafe_uptime() {
    if(active) {
        return MILLIS_PER_SEC * active->read() / active->freq;
    }

    return 0;
}

uint64_t uptime() {
    uint32_t flags;
    spin_lock_irqsave(&clock_lock, &flags);

    uint64_t ret = _unsafe_uptime();

    spin_unlock_irqstore(&clock_lock, flags);

    return ret;
}

void sleep(uint32_t milis) {
    uint32_t flags;
    spin_lock_irqsave(&clock_lock, &flags);

    //fixme this sucks
    uint64_t then = _unsafe_uptime();
    uint64_t now = _unsafe_uptime();
    while(now - then < milis) {
        if(!active) {
            panicf("sleep with active==NULL");
        }

        now = _unsafe_uptime();
    }


    spin_unlock_irqstore(&clock_lock, flags);
}

static INITCALL clock_init() {
    if(!active_event_source) panicf("no registered clock event source");

    active_event_source->event = handle_clock_event;

    return 0;
}

subsys_initcall(clock_init);
