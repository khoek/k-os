#include "mm/mm.h"
#include "fs/vfs.h"
#include "fs/binfmt.h"
#include "fs/exec.h"
#include "log/log.h"

bool execute_path(path_t *p, char **argv, char **envp) {
    file_t *f = vfs_open_file(p->dentry->inode);
    if(!f) return false;

    return binfmt_load(f);
}
