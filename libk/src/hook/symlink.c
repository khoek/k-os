#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int symlink (const char *path1, const char *path2) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("symlink");
    return -1;
}
