#include <sys/utsname.h>
#include <k/sys.h>

int uname(struct utsname *name) {
    return MAKE_SYSCALL(unimplemented, "uname", true);
}
