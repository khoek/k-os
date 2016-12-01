#include <k/sys.h>
#include <unistd.h>

unsigned int sleep(unsigned int seconds) {
    _msleep(seconds * 1000);

    return 0;
}
