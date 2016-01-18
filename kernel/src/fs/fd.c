#include "init/initcall.h"
#include "sync/spinlock.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "fs/fd.h"
#include "log/log.h"
#include "misc/stats.h"

#define NUM_ENTRIES 256

#define FREELIST_END ((uint32_t) 77)

static file_t *gfdt;
static uint32_t gfd_list[NUM_ENTRIES];
static uint32_t gfd_next;
static DEFINE_SPINLOCK(gfdt_lock);

static uint32_t file_to_idx(file_t *f) {
    return (((uint32_t) f) - ((uint32_t) gfdt)) / sizeof(file_t);
}

file_t * gfdt_obtain() {
    uint32_t flags;
    spin_lock_irqsave(&gfdt_lock, &flags);

    if(gfd_next == FREELIST_END) return NULL;

    gfdt_entries_in_use++;

    uint32_t added = gfd_next;
    gfd_next = gfd_list[gfd_next];

    spin_unlock_irqstore(&gfdt_lock, flags);

    gfdt[added].refs = 0;

    return &gfdt[added];
}

void gfdt_get(file_t *f) {
    uint32_t flags;
    spin_lock_irqsave(&gfdt_lock, &flags);

    f->refs++;

    spin_unlock_irqstore(&gfdt_lock, flags);
}

static void gfdt_free(file_t *f) {
    uint32_t flags;
    spin_lock_irqsave(&gfdt_lock, &flags);

    f->ops->close(f);

    uint32_t gfd = file_to_idx(f);
    gfd_list[gfd] = gfd_next;
    gfd_next = gfd;

    gfdt_entries_in_use--;

    spin_unlock_irqstore(&gfdt_lock, flags);
}

void gfdt_put(file_t *f) {
    uint32_t flags;
    spin_lock_irqsave(&gfdt_lock, &flags);

    f->refs--;

    if(!f->refs) {
        gfdt_free(f);
    }

    spin_unlock_irqstore(&gfdt_lock, flags);
}

static INITCALL gfdt_init() {
    gfdt = map_page(page_to_phys(alloc_pages(NUM_ENTRIES, 0)));
    gfd_next = 0;

    for(uint32_t i = 0; i < NUM_ENTRIES - 1; i++) {
        gfd_list[i] = i + 1;
    }
    gfd_list[NUM_ENTRIES - 1] = FREELIST_END;

    return 0;
}

core_initcall(gfdt_init);
