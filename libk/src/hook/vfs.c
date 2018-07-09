#include <sys/vfs.h>
#include <k/sys.h>

int statfs(const char *path, struct statfs *buf) {
    return MAKE_SYSCALL(unimplemented, "statfs", true);
}

int fstatfs(int fd, struct statfs *buf) {
    return MAKE_SYSCALL(unimplemented, "fstatfs", true);
}
