#ifndef KERNEL_NET_SOCK_H
#define KERNEL_NET_SOCK_H

#define AF_INET   1
#define AF_INET6  2
#define AF_UNIX   3
#define AF_UNSPEC 4
#define AF_LINK   5
#define AF_MAX    6

typedef enum sock_type {
    SOCK_STREAM = 1,
    SOCK_DGRAM  = 2,
    SOCK_RAW    = 3,
} sock_type_t;

#define SOCK_MAX (SOCK_RAW + 1)

#define SOCK_FLAG_CONNECTED (1 << 0)

#include "lib/int.h"
#include "sync/atomic.h"
#include "net/types.h"
#include "fs/fd.h"

typedef struct sock sock_t;

typedef struct sock_protocol sock_protocol_t;

typedef struct sock_family {
    uint32_t family;
    uint32_t addr_size;
    sock_protocol_t * (*find)(uint32_t, uint32_t);
} sock_family_t;

struct sock {
    sock_addr_t peer;
    uint32_t flags;

    sock_family_t *family;
    sock_protocol_t *proto;
    void *private;
};

struct sock_protocol {
    uint32_t type;

    void (*open)(sock_t *);
    uint32_t (*send)(sock_t *, void *buff, uint32_t len, uint32_t flags);
    void (*close)(sock_t *);

    /*
    void (*bind)(sock_t *, sock_addr_t *);
    void (*listen)(sock_t *);
    void (*connect)(sock_t *, sock_addr_t *);
    void (*accept)(sock_t *, sock_addr_t *);

    void (*sendto)(sock_t *);
    void (*read)(sock_t *);

    void (*recv)(sock_t *);
    void (*recvfrom)(sock_t *);
    void (*write)(sock_t *);

    void (*select)(sock_t *);
    void (*poll)(sock_t *);
    */
};

void register_sock_family(sock_family_t *family);

sock_t * sock_create(uint32_t family, uint32_t type, uint32_t protocol);
uint32_t sock_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags);
void sock_close(sock_t *sock);

gfd_idx_t sock_create_fd(uint32_t family, uint32_t type, uint32_t protocol);

static inline sock_t * gfd_to_sock(gfd_idx_t gfd_idx) {
    return (sock_t *) gfdt_get(gfd_idx)->private;
}

#endif
