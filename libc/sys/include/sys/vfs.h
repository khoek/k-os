#ifndef _SYS_STATFS_H
#define _SYS_STATFS_H

#include <sys/types.h>

//FIXME are these fields all the right size? (especially "f_fsid")
struct statfs {
    long int f_type;
#define f_fstyp f_type
    long int f_bsize;
    long int f_frsize;
    uint32_t f_blocks;
    uint32_t f_bfree;
    uint32_t f_files;
    uint32_t f_ffree;
    uint32_t f_bavail;

    uint32_t f_fsid;
    long int f_namelen;
    long int f_spare[6];
};

#endif
