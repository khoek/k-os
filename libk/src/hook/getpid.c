#include <unistd.h>

#include <sys/time.h>
#include <sys/times.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

pid_t getpid() {
    return SYSCALL(getpid)();
}
