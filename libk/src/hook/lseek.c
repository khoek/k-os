#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int lseek (int file, int ptr, int dir) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("lseek");
    return -1;
}
