#include <k/sys.h>

int getdents(int fd, void *buff, int count) {
    return MAKE_SYSCALL(getdents, fd, buff, count);
}
