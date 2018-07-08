#include <unistd.h>
#include <fcntl.h>

#include <k/compiler.h>
#include <k/sys.h>
#include <stdlib.h>
#include <string.h>

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

int execve(const char *filename, char *const argv[], char *const envp[]) {
    int fd = open(filename, 0);
    //TODO handle err conditions
    return fexecve(fd, argv, envp);
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

pid_t getpid() {
    return MAKE_SYSCALL(getpid);
}

pid_t getppid() {
    return MAKE_SYSCALL(getppid);
}

int chdir(const char *path) {
    return MAKE_SYSCALL(chdir, path);
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
    MAKE_SYSCALL(unimplemented, "getuid", false);
    return 0;
}

int setuid(uid_t uid) {
    return MAKE_SYSCALL(unimplemented, "setuid", true);
}

//FIXME unimplemented
gid_t getgid(void) {
    MAKE_SYSCALL(unimplemented, "getgid", false);
    return 0;
}

int setgid(gid_t uid) {
    return MAKE_SYSCALL(unimplemented, "setgid", true);
}

//FIXME unimplemented
uid_t geteuid() {
    MAKE_SYSCALL(unimplemented, "geteuid", false);
    return 0;
}

int setreuid(uid_t ruid, uid_t euid) {
    return MAKE_SYSCALL(unimplemented, "getreuid", true);
}

//FIXME unimplemented
gid_t getegid() {
    MAKE_SYSCALL(unimplemented, "getegid", false);
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

int dup(int fildes) {
    return MAKE_SYSCALL(unimplemented, "dup", true);
}

int dup2(int fildes, int fildes2) {
    return MAKE_SYSCALL(unimplemented, "dup2", true);
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
    return MAKE_SYSCALL(unimplemented, "chown", true);
}

off_t lseek(int file, off_t off, int whence) {
    return MAKE_SYSCALL(unimplemented, "lseek", true);
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
