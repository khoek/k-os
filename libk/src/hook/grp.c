#include <grp.h>

#include <k/sys.h>

void endgrent() {
    SYSCALL(unimplemented)("endgrent");
}

struct group *getgrent() {
    SYSCALL(unimplemented)("getgrent");
    return NULL;
}

void setgrent() {
    SYSCALL(unimplemented)("setgrent");
}
