#ifndef KERNEL_NET_SOCKET_H
#define KERNEL_NET_SOCKET_H

#define PF_INET 1

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#include "common/hashtable.h"

typedef struct sock sock_t;

typedef struct sock_provider {
    int (*socket)(int, sock_t *);
} sock_provider_t;

struct sock {
    sock_provider_t *provider;
    void *private;
};

typedef struct sock_type {
    int family, type;
    sock_provider_t * (*provider)(int);

    hlist_node_t node;
} sock_type_t;

void register_sock_type(sock_type_t *type);
void unregister_sock_type(sock_type_t *type);

#endif
