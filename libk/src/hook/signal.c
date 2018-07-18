#include <signal.h>
#include <k/sys.h>

static sigset_t do_restart;

int kill(int pid, int sig) {
    return MAKE_SYSCALL(kill, pid, sig);
}

int sigaction(int sig, const struct sigaction *restrict sa, struct sigaction *restrict sa_old) {
    return MAKE_SYSCALL(sigaction, sig, sa, sa_old);
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    return MAKE_SYSCALL(sigprocmask, how, set, oset);
}

int sigsuspend(const sigset_t *mask) {
    // FIXME for bash
    return MAKE_SYSCALL(unimplemented, "sigsuspend", false);
}

int siginterrupt(int sig, int flag) {
    struct sigaction sa;

    if(sigaction(sig, (struct sigaction *) NULL, &sa) < 0) {
        return -1;
    }

    if(flag) {
        sigaddset(&do_restart, sig);
        sa.sa_flags &= ~SA_RESTART;
    } else {
        sigdelset(&do_restart, sig);
        sa.sa_flags |= SA_RESTART;
    }

    if(sigaction(sig, &sa, (struct sigaction *) NULL) < 0) {
        return -1;
    }

    return 0;
}

int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    return MAKE_SYSCALL(unimplemented, "pthread_sigmask", true);
}

__sighandler_t signal(int sig, __sighandler_t handler) {
    struct sigaction sa, old_sa;
    sa.sa_handler = handler;
    sa.sa_flags = sigismember(&do_restart, sig) ? SA_RESTART : 0;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, sig);
    if(sigaction(sig, &sa, &old_sa) < 0) {
        return SIG_ERR;
    }
    return old_sa.sa_handler;
}

void _sigtramp(void *sp) {
    MAKE_SYSCALL(_sigreturn, sp);
}
