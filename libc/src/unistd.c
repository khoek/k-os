#include <unistd.h>

#include <k/sys.h>

ssize_t read(int fd, void *buf, size_t count) {
  return SYSCALL(read)(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
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
