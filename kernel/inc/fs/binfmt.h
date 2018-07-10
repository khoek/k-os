#ifndef KERNEL_FS_BINFMT_H
#define KERNEL_FS_BINFMT_H

#include "common/types.h"
#include "common/list.h"
#include "fs/vfs.h"

typedef struct binary {
    file_t *file;
    char **argv;
    char **envp;

    uint32_t iterations;
} binary_t;

typedef struct binfmt {
    list_head_t list;

    int32_t (*load)(binary_t *binary);
} binfmt_t;

binary_t * alloc_binary(file_t *file, char **argv, char **envp);

void binfmt_register(binfmt_t *binfmt);
int32_t binfmt_load(binary_t *binary);

#endif
