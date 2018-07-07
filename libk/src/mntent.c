#include <stdio.h>
#include <mntent.h>

FILE *setmntent(const char *filename, const char *type) {
    return fopen(filename, type);
}

struct mntent *getmntent(FILE *fp) {
    //FIXME coreutils wants this
    return NULL;
}

int endmntent(FILE *filep) {
    if(filep)	{
        fclose(filep);
    }
    return 1;
}
