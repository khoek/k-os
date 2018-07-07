#include <fcntl.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int open(const char *path, int oflag, ...) {
  return SYSCALL(open)(path, oflag);
}

int fcntl(int fildes, int cmd, ...) {
    //FIXME for bash
    //SYSCALL(unimplemented)("fcntl");
    errno = ENOSYS;
    return -1;
}
