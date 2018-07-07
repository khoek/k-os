#include <sgtty.h>

#include <k/sys.h>

int gtty(int fd, struct sgttyb *params) {
    SYSCALL(unimplemented)("gtty");
    return -1;
}

int stty(int fd, const struct sgttyb *params) {
    SYSCALL(unimplemented)("stty");
    return -1;
}
