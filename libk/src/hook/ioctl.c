// #include <stropts.h>
#include <k/sys.h>
#include <stdio.h>
#include <stdarg.h>

int ioctl(int fd, int request, ...) {
    va_list va;
    va_start(va, request);
    void *UNUSED(argp) = va_arg(va, void *);
    va_end(va);

    // printf("ioctl: %d, %X, %X\n", fd, request, (uint32_t) argp);

    return MAKE_SYSCALL(unimplemented, "ioctl", false);
}
