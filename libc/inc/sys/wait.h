#ifndef LIBC_SYS_WAIT_H
#define LIBC_SYS_WAIT_H

#include <inttypes.h>

pid_t wait(int *stat_loc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);

#endif
