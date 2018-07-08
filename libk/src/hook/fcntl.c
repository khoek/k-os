#include <fcntl.h>
#include <k/sys.h>

int open(const char *path, int oflag, ...) {
    return MAKE_SYSCALL(open, path, oflag);
}

int fcntl(int fildes, int cmd, ...) {
    //FIXME for bash
    return MAKE_SYSCALL(unimplemented, "fcntl", false);
}
