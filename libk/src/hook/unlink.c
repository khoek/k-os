#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int unlink (char *name) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("unlink");
    return -1;
}
