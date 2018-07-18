#include <sys/time.h>
#include <k/sys.h>

int gettimeofday(struct timeval *tv, void *tz) {
    return MAKE_SYSCALL(gettimeofday, tv);
}
