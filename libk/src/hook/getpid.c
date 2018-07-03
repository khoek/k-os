#include <sys/time.h>
#include <sys/times.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int getpid () {
    errno = ENOSYS;
    SYSCALL(unimplemented)("getpid");
    return -1;
}
