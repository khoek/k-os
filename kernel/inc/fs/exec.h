#ifndef KERNEL_FS_EXEC_H
#define KERNEL_FS_EXEC_H

uint32_t strtab_len(char *const tab[]);
char ** copy_strtab(char *const raw[]);

bool execute_path(dentry_t *d, char **argv, char **envp);

#endif
