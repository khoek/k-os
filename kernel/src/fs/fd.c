#include "init/initcall.h"
#include "sync/spinlock.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "fs/fd.h"
#include "log/log.h"
#include "misc/stats.h"

static cache_t *file_cache;
static DEFINE_SPINLOCK(gfdt_lock);

file_t * gfdt_obtain() {
    uint32_t flags;
    spin_lock_irqsave(&gfdt_lock, &flags);

    gfdt_entries_in_use++;

    file_t *f = cache_alloc(file_cache);

    spin_unlock_irqstore(&gfdt_lock, flags);

    f->refs = 0;

    return f;
}

void gfdt_get(file_t *f) {
    uint32_t flags;
    spin_lock_irqsave(&gfdt_lock, &flags);

    f->refs++;

    spin_unlock_irqstore(&gfdt_lock, flags);
}

static void gfdt_free(file_t *f) {
    f->ops->close(f);
    cache_free(file_cache, f);

    gfdt_entries_in_use--;
}

void gfdt_put(file_t *f) {
    uint32_t flags;
    spin_lock_irqsave(&gfdt_lock, &flags);

    if(f->refs) {
        f->refs--;
    }

    if(!f->refs) {
        gfdt_free(f);
    }

    spin_unlock_irqstore(&gfdt_lock, flags);
}

static INITCALL gfdt_init() {
    file_cache = cache_create(sizeof(file_t));

    return 0;
}

core_initcall(gfdt_init);
