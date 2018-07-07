

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int isatty(int fd) {
    //errno = ENOSYS;
    //FIXME for bash
    //SYSCALL(unimplemented)("isatty");

    //FIXME this is garbase
    return fd <= 2;
}
