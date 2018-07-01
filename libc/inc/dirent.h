#ifndef LIBC_DIRENT_H
#define LIBC_DIRENT_H

#include <sys/types.h>

struct dirent {
    ino_t d_ino;
    char d_name[];
};

typedef struct dir_desc {
    ufd_idx_t fd;
    uint32_t off;
} dir_desc_t;

#define DIR dir_desc_t

DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dir);

#endif
