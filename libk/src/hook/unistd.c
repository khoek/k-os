#include <unistd.h>
#include <fcntl.h>

#include <k/compiler.h>
#include <k/sys.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

void _exit(int status) {
    MAKE_SYSCALL(_exit, status);
    UNREACHABLE();
}

int read(int fd, void *buf, size_t count) {
    return MAKE_SYSCALL(read, fd, buf, count);
}

int write(int fd, const void *buf, size_t count) {
    return MAKE_SYSCALL(write, fd, buf, count);
}

int fsync(int fd) {
    return MAKE_SYSCALL(unimplemented, "fsync", true);
}

int fdatasync(int fd) {
    return MAKE_SYSCALL(unimplemented, "fdatasync", true);
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    return MAKE_SYSCALL(execve, filename, argv, envp);
}

int fexecve(int fd, char *const argv[], char *const envp[]) {
    return MAKE_SYSCALL(fexecve, fd, argv, envp);
}

pid_t fork() {
    return MAKE_SYSCALL(fork);
}

int close(int fildes) {
    return MAKE_SYSCALL(close, fildes);
}

int chdir(const char *path) {
    return MAKE_SYSCALL(chdir, path);
}

int fchdir(int fd) {
    return MAKE_SYSCALL(fchdir, fd);
}

static char my_cwd[256];
char *getcwd(char *buff, size_t len) {
    if(buff) {
        return (void *) MAKE_SYSCALL(getcwd, buff, len);
    } else {
        //FIXME for bash (terrible(?) hack)
        //FIXME dynamically determine size
        getcwd(my_cwd, 256); //FIXME handle error

        char *buff = malloc(strlen(my_cwd) + 1);
        strcpy(buff, my_cwd);
        return buff;
    }
}

#define HOSTNAME "k-os"
#define HOSTNAMELEN 4

int gethostname(char *name, size_t len) {
    // FIXME for bash
    MAKE_SYSCALL(unimplemented, "gethostname", false);

    uint32_t i;
    for(i = 0; (i < len - 1) && (i < HOSTNAMELEN + 1); i++) {
        name[i] = HOSTNAME[i];
    }
    if(len) {
        name[i] = 0;
    }
    return 0;
}

int sethostname(const char *name, size_t len) {
    return MAKE_SYSCALL(unimplemented, "sethostname", true);
}

//FIXME unimplemented
uid_t getuid() {
    return 0;
}

int setuid(uid_t uid) {
    return MAKE_SYSCALL(unimplemented, "setuid", true);
}

pid_t getpid() {
    return MAKE_SYSCALL(getpid);
}

pid_t getppid() {
    return MAKE_SYSCALL(getppid);
}

pid_t getpgid(pid_t pid) {
    return MAKE_SYSCALL(getpgid, pid);
}

int setpgid(pid_t pid, pid_t pgid) {
    return MAKE_SYSCALL(setpgid, pid, pgid);
}

pid_t getpgrp() {
    return getpgid(0);
}

pid_t setpgrp() {
    setpgid(0, 0);
    return getpid();
}

pid_t tcgetpgrp(int fd) {
    return MAKE_SYSCALL(tcgetpgrp, fd);
}

int tcsetpgrp(int fd, pid_t pgid) {
    return MAKE_SYSCALL(tcsetpgrp, fd, pgid);
}

//FIXME unimplemented
gid_t getgid(void) {
    return 0;
}

int setgid(gid_t uid) {
    return MAKE_SYSCALL(unimplemented, "setgid", true);
}

//FIXME unimplemented
uid_t geteuid() {
    return 0;
}

int setreuid(uid_t ruid, uid_t euid) {
    return MAKE_SYSCALL(unimplemented, "getreuid", true);
}

//FIXME unimplemented
gid_t getegid() {
    return 0;
}

int setregid(uid_t rgid, uid_t egid) {
    return MAKE_SYSCALL(unimplemented, "setregid", true);
}

unsigned int alarm(unsigned seconds) {
    return MAKE_SYSCALL(unimplemented, "alarm", true);
}

int pipe(int fildes[2]) {
    return MAKE_SYSCALL(unimplemented, "pipe", true);
}

int dup(int fd) {
    return MAKE_SYSCALL(dup, fd);
}

int dup2(int fd, int fd2) {
    return MAKE_SYSCALL(dup2, fd, fd2);
}

int access(const char *path, int amode) {
    return MAKE_SYSCALL(unimplemented, "access", true);
}

long sysconf(int name) {
    return MAKE_SYSCALL(unimplemented, "sysconf", false);
}

int isatty(int fd) {
    //FIXME for bash
    // return MAKE_SYSCALL(unimplemented, "isatty", true);

    //FIXME this is garbage
    return fd <= 2;
}

int chown(const char *path, uid_t owner, gid_t group) {
    return MAKE_SYSCALL(chown, path, owner, group);
}

int fchown(int fd, uid_t owner, gid_t group) {
    return MAKE_SYSCALL(fchown, fd, owner, group);
}

off_t lseek(int fd, off_t off, int whence) {
    return MAKE_SYSCALL(seek, fd, off, whence);
}

int link(const char *existing, const char *new) {
    return MAKE_SYSCALL(unimplemented, "link", true);
}

int unlink(const char *name) {
    return MAKE_SYSCALL(unimplemented, "unlink", true);
}

ssize_t readlink(const char *path, char *buf, size_t bufsize) {
    return MAKE_SYSCALL(unimplemented, "readlink", true);
}

int symlink(const char *path1, const char *path2) {
    return MAKE_SYSCALL(unimplemented, "symlink", true);
}

int getdtablesize() {
    //FIXME implement
    return INT_MAX - 1;
}

int truncate(const char *path, off_t length) {
    return MAKE_SYSCALL(unimplemented, "truncate", true);
}

int ftruncate(int fildes, off_t length) {
    return MAKE_SYSCALL(unimplemented, "ftruncate", true);
}

long pathconf(const char *path, int name) {
    return MAKE_SYSCALL(unimplemented, "pathconf", true);
}

long fpathconf(int fildes, int name) {
    return MAKE_SYSCALL(unimplemented, "fpathconf", true);
}

int nice(int incr) {
    return MAKE_SYSCALL(unimplemented, "nice", true);
}

int getpagesize() {
    return PAGE_SIZE;
}
