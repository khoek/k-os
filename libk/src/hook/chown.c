#include <sys/types.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int chown(const char *path, uid_t owner, gid_t group) {
    errno = ENOSYS;
    SYSCALL(unimplemented)("chown");
    return -1;
}
