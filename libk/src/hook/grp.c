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

struct group * getgrnam(const char *name) {
    MAKE_SYSCALL(unimplemented, "getgrnam", true);
    return NULL;
}

struct group * getgrgid(gid_t gid) {
    MAKE_SYSCALL(unimplemented, "getgrgid", true);
    return NULL;
}

int setgroups(size_t size, const gid_t *list) {
    return MAKE_SYSCALL(unimplemented, "setgroups", true);
}
