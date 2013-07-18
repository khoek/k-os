#include "common/init.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "fs/fd.h"
#include "video/log.h"

#define FREELIST_END (-1)

gfd_t *gfdt;
gfd_idx_t *gfd_list;
gfd_idx_t gfd_next;

gfd_idx_t gfdt_add(uint32_t type, uint32_t flags, fd_ops_t *ops, void *private) {
    BUG_ON(gfd_next == FREELIST_END);

    gfd_idx_t added = gfd_next;

    gfdt[added].type = type;
    gfdt[added].flags = flags;
    gfdt[added].ops = ops;
    gfdt[added].private = private;

    gfd_next = gfd_list[gfd_next];

    return added;
}

void gfdt_rm(gfd_idx_t gfd_idx) {
    gfd_list[gfd_idx] = gfd_next;
    gfd_next = gfd_idx;
}

static INITCALL gfdt_init() {
    gfdt = mm_map(page_to_phys(alloc_page(0)));
    gfd_list = mm_map(page_to_phys(alloc_page(0)));
    gfd_next = 0;

    for(uint32_t i = 0; i < (PAGE_SIZE / sizeof(gfd_t)) - 1; i++) {
        gfd_list[i] = i + 1;
    }
    gfd_list[(PAGE_SIZE / sizeof(gfd_t)) - 1] = FREELIST_END;

    return 0;
}

core_initcall(gfdt_init);
