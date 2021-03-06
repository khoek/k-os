#include "mm/mm.h"
#include "bug/debug.h"
#include "fs/vfs.h"
#include "fs/binfmt.h"
#include "fs/exec.h"

bool execute_path(path_t *path, char **raw_argv, char **raw_envp) {
    file_t *f = vfs_open_file(path);
    if(!f) {
        return false;
    }

    binary_t *bin = alloc_binary(f, copy_strtab(raw_argv), copy_strtab(raw_envp));

    return binfmt_load(bin);
}
