#include <stddef.h>

#include "list.h"
#include "common.h"
#include "binfmt.h"
#include "log.h"

static LIST_HEAD(binfmts);

void binfmt_register(binfmt_t *binfmt) {
    list_add(&binfmt->list, &binfmts);
}

int binfmt_load_exe(void *start, uint32_t length) {
    binfmt_t *binfmt;
    LIST_FOR_EACH_ENTRY(binfmt, &binfmts, list) {
        if(!(*binfmt->load_exe)(start, length)) return 0;
    }

    return -1;
}

int binfmt_load_lib(void UNUSED(*start), uint32_t UNUSED(length)) {
    return -1;
}
