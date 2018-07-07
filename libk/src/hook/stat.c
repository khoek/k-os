#include <sys/stat.h>

#include <errno.h>
#undef errno
extern int errno;

#include <k/sys.h>

int stat(const char *file, struct stat *st) {
    return SYSCALL(stat)(file, st);
}

int	mkfifo(const char *path, mode_t mode) {
    SYSCALL(unimplemented)("mkfifo");
    return -1;
}

mode_t umask(mode_t cmask) {
    SYSCALL(unimplemented)("umask");
    return 0;
}
