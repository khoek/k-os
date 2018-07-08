#include <sys/select.h>
#include <k/sys.h>

int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout) {
    //FIXME for bash
    return MAKE_SYSCALL(unimplemented, "select", false);
}
