#ifndef KERNEL_FS_EXEC_H
#define KERNEL_FS_EXEC_H

bool execute_path(path_t *p, char **argv, char **envp);

#endif
