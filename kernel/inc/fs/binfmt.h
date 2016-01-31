#ifndef KERNEL_FS_BINFMT_H
#define KERNEL_FS_BINFMT_H

#include "common/types.h"
#include "common/list.h"
#include "fs/vfs.h"

typedef struct binfmt {
    list_head_t list;

    bool (*load)(file_t *f, char **argv, char **envp);
} binfmt_t;

void binfmt_register(binfmt_t *binfmt);
bool binfmt_load(file_t *f, char **argv, char **envp);

#endif
