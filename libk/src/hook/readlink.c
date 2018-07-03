#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int readlink (const char *path, char *buf, size_t bufsize) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("readlink");
    return -1;
}
