#include <sys/select.h>

#include <k/sys.h>

int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout) {
    //errno = ENOSYS;
    //FIXME for bash
    //SYSCALL(unimplemented)("select");
    return -1;
}
