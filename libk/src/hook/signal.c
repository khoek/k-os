#include <signal.h>

#include <errno.h>
#undef errno
extern int errno;

#include "k/sys.h"

int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    // FIXME for bash
    // SYSCALL(unimplemented)("sigprocmask");
    errno = ENOSYS;
    return -1;
}

int sigsuspend(const sigset_t *mask) {
    // FIXME for bash
    // SYSCALL(unimplemented)("sigsuspend");
    errno = ENOSYS;
    return -1;
}

int sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact) {
    // FIXME for bash
    // SYSCALL(unimplemented)("sigaction");
    errno = ENOSYS;
    return -1;
}
