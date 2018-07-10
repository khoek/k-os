#include <sys/stat.h>
#include <k/sys.h>

int	mkfifo(const char *path, mode_t mode) {
    return MAKE_SYSCALL(unimplemented, "mkfifo", true);
}

mode_t umask(mode_t cmask) {
    return MAKE_SYSCALL(unimplemented, "umask", true);
}

int stat(const char *file, struct stat *st) {
    return MAKE_SYSCALL(stat, file, st);
}

int lstat(const char *file, struct stat *st) {
    return MAKE_SYSCALL(lstat, file, st);
}

int fstat(int fd, struct stat *st) {
    return MAKE_SYSCALL(fstat, fd, st);
}

int chmod(const char *pathname, mode_t mode) {
    return MAKE_SYSCALL(unimplemented, "chmod", true);
}

int fchmod(int fd, mode_t mode) {
    return MAKE_SYSCALL(unimplemented, "fchmod", true);
}

int mkdir(const char *pathname, mode_t mode) {
    return MAKE_SYSCALL(unimplemented, "mkdir", true);
}

int rmdir(const char *pathname) {
    return MAKE_SYSCALL(unimplemented, "rmdir", true);
}
