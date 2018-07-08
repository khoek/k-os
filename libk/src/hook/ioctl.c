// #include <stropts.h>
#include <k/sys.h>

int ioctl(int fildes, int request, ... /* arg */) {
    //FIXME for bash
    return MAKE_SYSCALL(unimplemented, "ioctl", false);
}
