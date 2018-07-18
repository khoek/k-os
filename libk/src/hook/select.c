#include <sys/select.h>
#include <k/sys.h>

int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout) {
    return MAKE_SYSCALL(select, nfds, readfds, writefds, errorfds, timeout);
}
