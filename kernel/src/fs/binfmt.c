#include "common/list.h"
#include "common/compiler.h"
#include "bug/debug.h"
#include "fs/binfmt.h"
#include "log/log.h"

static DEFINE_LIST(binfmts);

void binfmt_register(binfmt_t *binfmt) {
    list_add(&binfmt->list, &binfmts);
}

bool binfmt_load(file_t *file, char **argv, char **envp) {
    binfmt_t *binfmt;
    LIST_FOR_EACH_ENTRY(binfmt, &binfmts, list) {
        if(binfmt->load(file, argv, envp)) {
            //If successful, this should never return
            BUG();
        }
    }

    return false;
}
