#ifndef KERNEL_FS_EXEC_H
#define KERNEL_FS_EXEC_H

uint32_t strtab_len(char **tab);
char ** copy_strtab(char **raw);

bool execute_path(path_t *p, char **argv, char **envp);

#endif
