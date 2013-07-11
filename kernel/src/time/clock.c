#include <stddef.h>

#include "common/list.h"
#include "common/init.h"
#include "common/asm.h"
#include "bug/panic.h"
#include "time/clock.h"

static DEFINE_LIST(clocks);
static DEFINE_LIST(clock_event_sources);
static DEFINE_LIST(clock_event_listeners);

static clock_t *active;
static clock_event_source_t *active_event_source;

static void handle_clock_event(clock_event_source_t *clock_event_source) {
    clock_event_listener_t *listener;
    LIST_FOR_EACH_ENTRY(listener, &clock_event_listeners, list) {
        listener->handle(clock_event_source);
    }
}

static void handle_clock_nop(clock_event_source_t *clock_event_source) {
}

void register_clock(clock_t *clock) {
    if(!active || active->rating < clock->rating) {
        active = clock;
    }

    list_add(&clock->list, &clocks);
}

void register_clock_event_source(clock_event_source_t *clock_event_source) {
    if(!active_event_source || active_event_source->rating < clock_event_source->rating) {
        active_event_source = clock_event_source;
    }
    clock_event_source->event = handle_clock_nop;

    list_add(&clock_event_source->list, &clock_event_sources);
}

void register_clock_event_listener(clock_event_listener_t *clock_event_listener) {
    list_add(&clock_event_listener->list, &clock_event_listeners);
}

uint64_t uptime() {
    if(!active) return 0;

    return active->read();
}

void sleep(uint32_t milis) {
    uint64_t then = uptime();
    while(uptime() - then < milis) hlt(); //FIXME hlt might not work here (or will definitely be inaccurate)
}

static INITCALL init_clock() {
    if(!active_event_source) panicf("No registered clock event source");

    active_event_source->event = handle_clock_event;

    return 0;
}

subsys_initcall(init_clock);
