#include <stddef.h>

#include "common.h"
#include "binfmt.h"

binfmt_t *binfmt_front, *binfmt_back;

void binfmt_register(binfmt_t *binfmt) {
    binfmt->next = NULL;

    if(!binfmt_front) {
        binfmt->prev = NULL;
        binfmt_front = binfmt_back = binfmt;
    } else {
        binfmt->prev = binfmt_back;
        binfmt_back->next = binfmt;
        binfmt_back = binfmt;
    }
}

int binfmt_load_exe(void *start, uint32_t length) {
    for(binfmt_t *binfmt = binfmt_front; binfmt != NULL; binfmt = binfmt->next) {
        if(!(*binfmt->load_exe)(start, length)) return 0;
    }

    return -1;
}

int binfmt_load_lib(void UNUSED(*start), uint32_t UNUSED(length)) {
    return -1;
}
