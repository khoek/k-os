#ifndef KERNEL_BINFMT_H
#define KERNEL_BINFMT_H

#include "int.h"

typedef struct binfmt binfmt_t;

struct binfmt {
    binfmt_t *prev, *next;
    int (*load_exe)(void *start, uint32_t length);
    int (*load_lib)(void *start, uint32_t length);
};

void binfmt_register(binfmt_t *binfmt);

int binfmt_load_exe(void *start, uint32_t length);
int binfmt_load_lib(void *start, uint32_t length);

#endif
