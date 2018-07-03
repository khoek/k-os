#include <sys/times.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

clock_t times (struct tms *buf) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("times");
    return -1;
}
