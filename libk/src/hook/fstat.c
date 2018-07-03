#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int fstat (int fildes, struct stat *st) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("fstat");
    return -1;
}
