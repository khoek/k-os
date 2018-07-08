#include <signal.h>
#include <k/sys.h>

int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    // FIXME for bash
    return MAKE_SYSCALL(unimplemented, "sigprocmask", false);
}

int sigsuspend(const sigset_t *mask) {
    // FIXME for bash
    return MAKE_SYSCALL(unimplemented, "sigsuspend", false);
}

int sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact) {
    // FIXME for bash
    return MAKE_SYSCALL(unimplemented, "sigaction", false);
}
