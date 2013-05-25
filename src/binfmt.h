#include "vfs.h"

typedef struct binfmt {
    int (*load_exe)(file_t *file);
    int (*load_lib)(file_t *file);
} binfmt_t;

void binfmt_register(binfmt_t *binfmt);
