#include <grp.h>
#include <k/sys.h>

void endgrent() {
    MAKE_SYSCALL(unimplemented, "endgrent", true);
}

struct group *getgrent() {
    MAKE_SYSCALL(unimplemented, "getgrent", true);
    return NULL;
}

void setgrent() {
    MAKE_SYSCALL(unimplemented, "setgrent", true);
}
