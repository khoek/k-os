#include <stddef.h>

#include "common/list.h"
#include "common/compiler.h"
#include "fs/binfmt.h"
#include "video/log.h"

static DEFINE_LIST(binfmts);

void binfmt_register(binfmt_t *binfmt) {
    list_add(&binfmt->list, &binfmts);
}

int binfmt_load_exe(const char *name, void *start, uint32_t length) {
    binfmt_t *binfmt;
    LIST_FOR_EACH_ENTRY(binfmt, &binfmts, list) {
        if(!(*binfmt->load_exe)(name, start, length)) return 0;
    }

    return -1;
}
