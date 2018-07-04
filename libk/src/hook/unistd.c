#include <unistd.h>
#include <fcntl.h>
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
    int fd = open(filename, 0);
    //TODO handle err conditions
    return fexecve(fd, argv, envp);
}

int fexecve(int fd, char *const argv[], char *const envp[]) {
    return SYSCALL(fexecve)(fd, argv, envp);
}

pid_t fork() {
    return SYSCALL(fork)();
}

int close(int fildes) {
    return SYSCALL(close)(fildes);
}

int chdir(const char *path) {
    return SYSCALL(chdir)(path);
}

char *getcwd(char *buff, size_t len) {
    return (void *) SYSCALL(getcwd)(buff, len);
}
