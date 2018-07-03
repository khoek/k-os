#include <unistd.h>
#include <errno.h>
#undef errno
extern int errno;

#include <k/compiler.h>
#include <k/sys.h>

void _exit(int status) {
    SYSCALL(_exit)(status);
    UNREACHABLE();
}

int read(int fd, void *buf, size_t count) {
    return SYSCALL(read)(fd, buf, count);
}

int write(int fd, const void *buf, size_t count) {
    return SYSCALL(write)(fd, buf, count);
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    return SYSCALL(execve)(filename, argv, envp);
}

pid_t fork() {
    return SYSCALL(fork)();
}

int close(int fildes) {
    return SYSCALL(close)(fildes);
}
