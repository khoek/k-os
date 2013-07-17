#ifndef KERNEL_NET_SOCK_H
#define KERNEL_NET_SOCK_H

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

#define SOCK_MAX (SOCK_RAW + 1)

#include "lib/int.h"
#include "sync/atomic.h"
#include "net/types.h"
#include "fs/fd.h"

typedef struct sock sock_t;

typedef struct sock_protocol sock_protocol_t;

typedef struct sock_family {
    uint32_t code;
    uint32_t addr_size;
    sock_protocol_t * (*find)(uint32_t, uint32_t);
} sock_family_t;

struct sock {
    sock_addr_t peer;

    sock_family_t *family;
    sock_protocol_t *proto;
    void *private;
};

struct sock_protocol {
    void (*open)(sock_t *);
    void (*close)(sock_t *);

    /*
    void (*bind)(sock_t *, sock_addr_t *);
    void (*listen)(sock_t *);
    void (*connect)(sock_t *, sock_addr_t *);
    void (*accept)(sock_t *, sock_addr_t *);

    void (*send)(sock_t *);
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
void sock_close(sock_t *sock);

gfd_idx_t sock_create_fd(uint32_t family, uint32_t type, uint32_t protocol);

#endif
