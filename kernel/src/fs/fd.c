#include "common/init.h"
#include "sync/spinlock.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "fs/fd.h"
#include "video/log.h"
#include "misc/stats.h"

#define FREELIST_END (-1)

gfd_t *gfdt;
gfd_idx_t *gfd_list;
gfd_idx_t gfd_next;
gfd_idx_t gfd_max;
DEFINE_SPINLOCK(gfd_lock);

gfd_idx_t gfdt_add(file_t *file) {
    uint32_t flags;
    spin_lock_irqsave(&gfd_lock, &flags);
    
    gfds_in_use++;

    BUG_ON(gfd_next == FREELIST_END);

    gfd_idx_t added = gfd_next;

    gfdt[added].refs = 1;
    gfdt[added].file = file;

    gfd_next = gfd_list[gfd_next];
    
    spin_unlock_irqstore(&gfd_lock, flags);

    return added;
}

file_t * gfdt_get(gfd_idx_t gfd) {
    uint32_t flags;
    spin_lock_irqsave(&gfd_lock, &flags);

    BUG_ON(gfd > gfd_max);
    if(!gfdt[gfd].file) return NULL;
    
    gfdt[gfd].refs++;
    
    spin_unlock_irqstore(&gfd_lock, flags);

    return gfdt[gfd].file;
}

void gfdt_put(gfd_idx_t gfd) {
    uint32_t flags;
    spin_lock_irqsave(&gfd_lock, &flags);

    BUG_ON(gfd > gfd_max);
    BUG_ON(!gfdt[gfd].file);
    
    gfdt[gfd].refs--;
    
    if(!gfdt[gfd].refs) {    
        gfdt[gfd].file->ops->close(gfdt[gfd].file);
        gfdt[gfd].file = NULL; //FIXME do we need to free gfdt[gfd].file?
        
        gfd_list[gfd] = gfd_next;
        gfd_next = gfd;
        
        gfds_in_use--;
    }
    
    spin_unlock_irqstore(&gfd_lock, flags);
}

static INITCALL gfdt_init() {
    gfdt = mm_map(page_to_phys(alloc_page(0)));
    gfd_list = mm_map(page_to_phys(alloc_page(0)));
    gfd_next = 0;
    gfd_max = (PAGE_SIZE / sizeof(gfd_t)) - 1;

    for(gfd_idx_t i = 0; i < gfd_max; i++) {
        gfd_list[i] = i + 1;
    }
    gfd_list[gfd_max] = FREELIST_END;

    return 0;
}

core_initcall(gfdt_init);
