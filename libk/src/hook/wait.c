#include <sys/wait.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

pid_t wait(int *stat_loc) {
    return waitpid((pid_t) -1, stat_loc, 0);
}

pid_t waitpid(pid_t pid, int *stat_loc, int options) {
  return SYSCALL(waitpid)(pid, stat_loc, options);
}
