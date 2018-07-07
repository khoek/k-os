

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int ioctl(int fildes, int request, ... /* arg */) {
    //FIXME for bash
    //SYSCALL(unimplemented)("ioctl");
    errno = ENOSYS;
    return -1;
}
