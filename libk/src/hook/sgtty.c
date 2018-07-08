#include <sgtty.h>
#include <k/sys.h>

int gtty(int fd, struct sgttyb *params) {
    return MAKE_SYSCALL(unimplemented, "gtty", true);
}

int stty(int fd, const struct sgttyb *params) {
    return MAKE_SYSCALL(unimplemented, "stty", true);
}
