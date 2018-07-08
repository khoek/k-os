#include <sys/time.h>
#include <sys/times.h>
#include <k/sys.h>

int gettimeofday(struct timeval *ptimeval, void *ptimezone) {
    //FIXME for bash
    return MAKE_SYSCALL(unimplemented, "gettimeofday", false);
}
