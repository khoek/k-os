#ifndef K_SYS_SOCKET_H
#define K_SYS_SOCKET_H

#include "k/int.h"

#define PF_INET 1
#define PF_LINK 2
#define PF_MAX  3

#define AF_INET PF_INET
#define AF_LINK PF_LINK
#define AF_MAX  PF_MAX

typedef enum sock_type {
    SOCK_STREAM = 1,
    SOCK_DGRAM  = 2,
    SOCK_RAW    = 3,
} sock_type_t;

typedef uint32_t socklen_t;
typedef unsigned int sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[];
};

#endif
