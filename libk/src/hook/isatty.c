#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int isatty (int file) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("isatty");
    return -1;
}
