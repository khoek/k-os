#include <sys/times.h>
#include <k/sys.h>

clock_t times (struct tms *buf) {
    return MAKE_SYSCALL(unimplemented, "times", true);
}
