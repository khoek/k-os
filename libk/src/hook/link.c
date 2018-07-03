#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int link (char *existing, char *new) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("link");
    return -1;
}
