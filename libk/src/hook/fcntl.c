#include <fcntl.h>
#include <stdarg.h>
#include <k/sys.h>

int open(const char *path, int oflag, ...) {
    int mode = 0;
    if(oflag & O_CREAT) {
        va_list va;
        va_start(va, oflag);
        mode = va_arg(va, int);
        va_end(va);
    }
    return MAKE_SYSCALL(open, path, oflag, mode);
}

int fcntl(int fildes, int cmd, ...) {
    //FIXME for bash
    return MAKE_SYSCALL(unimplemented, "fcntl", false);
}
