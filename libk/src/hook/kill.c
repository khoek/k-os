#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int kill (int pid, int sig) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("kill");
    return -1;
}
