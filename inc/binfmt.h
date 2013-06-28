#ifndef KERNEL_BINFMT_H
#define KERNEL_BINFMT_H

#include "int.h"
#include "list.h"

typedef struct binfmt {
    list_head_t list;

    int (*load_exe)(void *start, uint32_t length);
    int (*load_lib)(void *start, uint32_t length);
} binfmt_t;

void binfmt_register(binfmt_t *binfmt);

int binfmt_load_exe(void *start, uint32_t length);
int binfmt_load_lib(void *start, uint32_t length);

#endif
