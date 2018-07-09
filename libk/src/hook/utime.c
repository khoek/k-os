#include <utime.h>
#include <k/sys.h>

int utime(const char *path, const struct utimbuf *times) {
    return MAKE_SYSCALL(unimplemented, "utime", true);
}
