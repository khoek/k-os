#include <k/sys.h>

int getdents(int fd, void *buff, int count) {
    return SYSCALL(getdents)(fd, buff, count);
}
