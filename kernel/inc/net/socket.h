#ifndef KERNEL_NET_SOCKET_H
#define KERNEL_NET_SOCKET_H

#define PF_INET 1
#define PF_MAX  2

#define AF_INET PF_INET
#define AF_MAX  PF_MAX

typedef enum socket_type {
    SOCK_STREAM = 1,
    SOCK_DGRAM  = 2,
    SOCK_RAW    = 3,
} socket_type_t;

#define SOCK_MAX (SOCK_RAW + 1)

#include "lib/int.h"
#include "sync/atomic.h"

typedef struct socket socket_t;

typedef struct socket_protocol socket_protocol_t;

typedef struct socket_family {
    uint32_t code;
    socket_protocol_t * (*create)(uint32_t, uint32_t);
} socket_family_t;

struct socket {
    socket_protocol_t *proto;
    void *private;
};

struct socket_protocol {
    void (*create)(socket_t *);
    void (*destroy)(socket_t *);
};

void register_socket_family(socket_family_t *family);
socket_t * socket_create(uint32_t family, uint32_t type, uint32_t protocol);

#endif
