#ifndef KERNEL_INIT_PARAM_H
#define KERNEL_INIT_PARAM_H

#include <stdbool.h>

typedef struct cmdline_param cmdline_param_t;

typedef bool (*param_handler_t)(char *);

struct cmdline_param {
    char *name;
    param_handler_t handle;
};

void parse_cmdline();

#define cmdline_param(key, func)                      \
    static cmdline_param_t param_##func               \
    __attribute__((section(".init.param"), used)) = { \
        .name = key,                                  \
        .handle = func,                               \
    }

#endif
