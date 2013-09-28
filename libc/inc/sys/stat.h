#ifndef LIBC_SYS_STAT_H
#define LIBC_SYS_STAT_H

#include <sys/types.h>

struct stat {

};

int mkdir(const char *, mode_t);
int fstat(int, struct stat *);
int stat(const char *restrict, struct stat *restrict);

#endif
