#include <grp.h>
#include <k/sys.h>

#include <stdlib.h>

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

char * root_mem[] = {"root", NULL};

//FIXME read this from "/etc/group"
static struct group GROUPS[] = {
    {
      .gr_name = "root",
      .gr_passwd = "x",
      .gr_gid = 0,
      .gr_mem = root_mem,
    }
};

struct group * getgrgid(gid_t gid) {
    MAKE_SYSCALL(unimplemented, "getgrgid", false);
    for(uint32_t i = 0; i < sizeof(GROUPS) / sizeof(struct group); i++) {
        if(GROUPS[i].gr_gid == gid) {
            struct group *g = malloc(sizeof(struct group));
            *g = GROUPS[i];
            return g;
        }
    }
    return NULL;
}

int setgroups(size_t size, const gid_t *list) {
    return MAKE_SYSCALL(unimplemented, "setgroups", true);
}
