#include <sys/time.h>
#include <sys/times.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int gettimeofday (struct timeval *ptimeval, void *ptimezone) {
    errno = ENOSYS;
    //FIXME for bash
    //SYSCALL(unimplemented)("gettimeofday");
    return -1;
}
