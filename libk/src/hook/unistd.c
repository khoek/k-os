#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#undef errno
extern int errno;

#include <k/compiler.h>
#include <k/sys.h>
#include <stdlib.h>
#include <string.h>

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

pid_t getpid() {
    return SYSCALL(getpid)();
}

pid_t getppid() {
    return SYSCALL(getppid)();
}

int chdir(const char *path) {
    return SYSCALL(chdir)(path);
}

static char my_cwd[256];
char *getcwd(char *buff, size_t len) {
    if(buff) {
        return (void *) SYSCALL(getcwd)(buff, len);
    } else {
        //FIXME for bash (terrible(?) hack)
        //FIXME dynamically determine size
        getcwd(my_cwd, 256); //FIXME handle error

        //FIXME SUPER TERRIBLE HACK NOW!
        // char *buff = malloc(strlen(my_cwd) + 1);
        // strcpy(buff, my_cwd);
        // return buff;

        return my_cwd;
    }
}

#define HOSTNAME "k-os"
#define HOSTNAMELEN 4

int gethostname(char *name, size_t len) {
    // FIXME for bash
    // SYSCALL(unimplemented)("gethostname");

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
    SYSCALL(unimplemented)("sethostname");
    return -1;
}

//FIXME unimplemented
uid_t getuid() {
    return 0;
}

int setuid(uid_t uid) {
    SYSCALL(unimplemented)("setuid");
    return -1;
}

//FIXME unimplemented
gid_t getgid(void) {
    return 0;
}

int setgid(gid_t uid) {
    SYSCALL(unimplemented)("setgid");
    return -1;
}

//FIXME unimplemented
uid_t geteuid() {
    return 0;
}

int setreuid(uid_t ruid, uid_t euid) {
    SYSCALL(unimplemented)("setreuid");
    return -1;
}

//FIXME unimplemented
gid_t getegid() {
    return 0;
}

int setregid(uid_t rgid, uid_t egid) {
    SYSCALL(unimplemented)("setregid");
    return -1;
}

unsigned int alarm(unsigned seconds) {
    SYSCALL(unimplemented)("alarm");
    return -1;
}

int pipe(int fildes[2]) {
    SYSCALL(unimplemented)("pipe");
    return -1;
}

int dup(int fildes) {
    SYSCALL(unimplemented)("dup");
    return -1;
}

int dup2(int fildes, int fildes2) {
    SYSCALL(unimplemented)("dup2");
    return -1;
}

int access(const char *path, int amode) {
    SYSCALL(unimplemented)("access");
    return -1;
}

long sysconf(int name) {
    //SYSCALL(unimplemented)("sysconf");
    errno = ENOSYS;
    return -1;
}
