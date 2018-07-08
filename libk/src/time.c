#include <k/sys.h>

void _msleep(uint32_t millis) {
    MAKE_SYSCALL(msleep, millis);
}

unsigned int sleep(unsigned int seconds) {
    _msleep(seconds * 1000);

    return 0;
}
