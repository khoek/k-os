#include "common/list.h"
#include "common/compiler.h"
#include "bug/debug.h"
#include "fs/binfmt.h"
#include "log/log.h"

#define MAX_LOAD_DEPTH 5

static DEFINE_LIST(binfmts);

void binfmt_register(binfmt_t *binfmt) {
    list_add(&binfmt->list, &binfmts);
}

binary_t * alloc_binary(file_t *file, char **argv, char **envp) {
    binary_t *bin = kmalloc(sizeof(binary_t));
    bin->file = file;
    bin->argv = argv;
    bin->envp = envp;
    bin->iterations = 0;
    return bin;
}

int32_t binfmt_load(binary_t *binary) {
    if(binary->iterations > MAX_LOAD_DEPTH) {
        return -ELOOP;
    }

    binary->iterations++;

    binfmt_t *binfmt;
    LIST_FOR_EACH_ENTRY(binfmt, &binfmts, list) {
        int32_t ret = binfmt->load(binary);
        if(ret != -ENOEXEC) {
            BUG_ON(!ret);
            return ret;
        }
    }

    binary->iterations--;

    return -ENOEXEC;
}
