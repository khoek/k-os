#include "mm/mm.h"
#include "bug/debug.h"
#include "fs/vfs.h"
#include "fs/binfmt.h"
#include "fs/exec.h"

uint32_t strtab_len(char **tab) {
    if(!tab) return 0;

    uint32_t len = 0;
    while(*(tab++)) {
        len++;
    }
    return len;
}

char ** copy_strtab(char **raw) {
    if(!raw) return NULL;

    uint32_t len = strtab_len(raw);

    char **copy = kmalloc(sizeof(char *) * (len + 1));
    for(uint32_t i = 0; i < len; i++) {
        copy[i] = strdup(raw[i]);
    }
    copy[len] = NULL;

    return copy;
}

bool execute_path(path_t *p, char **raw_argv, char **raw_envp) {
    file_t *f = vfs_open_file(p->dentry->inode);
    if(!f) return false;

    if(!binfmt_load(f, copy_strtab(raw_argv), copy_strtab(raw_envp))) {
        return false;
    }

    BUG();
}
