#ifndef KERNEL_NET_SOCK_H
#define KERNEL_NET_SOCK_H

#include <stdbool.h>

#include "lib/int.h"

#define AF_INET   1
#define AF_INET6  2
#define AF_UNIX   3
#define AF_UNSPEC 4
#define AF_LINK   5
#define AF_MAX    6

#define SHUT_RD   1
#define SHUT_RDWR 2
#define SHUT_WR   3
#define SHUT_MASK 3

typedef enum sock_type {
    SOCK_STREAM = 1,
    SOCK_DGRAM  = 2,
    SOCK_RAW    = 3,
} sock_type_t;

#define SOCK_MAX (SOCK_RAW + 1)

#define SOCK_FLAG_CONNECTED (1 << 0)
#define SOCK_FLAG_SHUT_RD   (1 << 1)
#define SOCK_FLAG_SHUT_WR   (1 << 2)
#define SOCK_FLAG_SHUT_RDWR (SOCK_FLAG_SHUT_RD | SOCK_FLAG_SHUT_WR)
#define SOCK_FLAG_BOUND     (1 << 3)
#define SOCK_FLAG_LISTENING (1 << 4)

typedef uint32_t socklen_t;
typedef unsigned int sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[];
};

typedef struct sock_addr {
    uint32_t family;
    void *addr;
} sock_addr_t;

typedef uint32_t net_protocol_t;

typedef struct sock sock_t;

typedef struct sock_protocol sock_protocol_t;

typedef struct sock_route {
    sock_addr_t src;
    sock_addr_t dst;

    net_protocol_t protocol;
} sock_route_t;

typedef struct sock_buff {
    uint32_t size;
    void *buff;
} sock_buff_t;

typedef struct sock_family {
    uint32_t family;
    uint32_t addr_len;
    sock_protocol_t * (*find)(uint32_t, uint32_t);
} sock_family_t;

struct sock {
    sock_addr_t peer;
    sock_addr_t local;
    uint32_t flags;

    sock_family_t *family;
    sock_protocol_t *proto;
    void *private;
};

#include "lib/int.h"
#include "net/packet.h"
#include "fs/fd.h"

struct sock_protocol {
    uint32_t type;

    void (*open)(sock_t *);
    void (*close)(sock_t *);
    bool (*listen)(sock_t *, uint32_t backlog);
    bool (*bind)(sock_t *, sock_addr_t *);
    bool (*connect)(sock_t *, sock_addr_t *);
    bool (*shutdown)(sock_t *, int);
    uint32_t (*send)(sock_t *, void *buff, uint32_t len, uint32_t flags);
    uint32_t (*recv)(sock_t *, void *buff, uint32_t len, uint32_t flags);

    /*
    void (*bind)(sock_t *, sock_addr_t *);
    void (*listen)(sock_t *);
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
sock_t * sock_open(sock_family_t *family, sock_protocol_t *protocol);
bool sock_listen(sock_t *sock, uint32_t backlog);
bool sock_bind(sock_t *sock, sock_addr_t *addr);
bool sock_connect(sock_t *sock, sock_addr_t *addr);
bool sock_shutdown(sock_t *sock, int how);
uint32_t sock_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags);
uint32_t sock_recv(sock_t *sock, void *buff, uint32_t len, uint32_t flags);
void sock_close(sock_t *sock);

gfd_idx_t sock_create_fd(uint32_t family, uint32_t type, uint32_t protocol);

static inline sock_t * gfd_to_sock(gfd_idx_t gfd_idx) {
    return (sock_t *) gfdt_get(gfd_idx)->private;
}

#endif
