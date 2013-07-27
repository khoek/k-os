#include "lib/int.h"
#include "lib/string.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "net/socket.h"
#include "fs/fd.h"
#include "video/log.h"

static sock_family_t *families[AF_MAX];
static DEFINE_SPINLOCK(family_lock);

void register_sock_family(sock_family_t *family) {
    uint32_t flags;
    spin_lock_irqsave(&family_lock, &flags);

    if(families[family->family]) panicf("Socket family %u already registered!", family->family);
    families[family->family] = family;

    spin_unlock_irqstore(&family_lock, flags);
}

static sock_t * sock_alloc() {
    sock_t *alloc = kmalloc(sizeof(sock_t));
    memset(alloc, 0, sizeof(sock_t));

    return alloc;
}

static void sock_free(sock_t *sock) {
    kfree(sock, sizeof(sock_t));
}

sock_t * sock_create(uint32_t family, uint32_t type, uint32_t protocol) {
    uint32_t flags;
    spin_lock_irqsave(&family_lock, &flags);

    if(!families[family]) {
        return NULL;
    }

    sock_t *sock = NULL;
    sock_protocol_t *proto = families[family]->find(type, protocol);
    if(proto) {
        sock = sock_alloc();
        sock->family = families[family];
        sock->proto = proto;

        sock->proto->open(sock);
    }

    spin_unlock_irqstore(&family_lock, flags);

    return sock;
}

bool sock_connect(sock_t *sock, sock_addr_t *addr) {
    if(!(sock->proto->type == SOCK_DGRAM || sock->proto->type == SOCK_RAW)) {
        if(sock->flags & SOCK_FLAG_CONNECTED) {
            //FIXME errno = EISCONN
            return false;
        }
    }

    return sock->proto->connect(sock, addr);
}

uint32_t sock_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    if(sock->flags & SOCK_FLAG_CONNECTED) {
        return sock->proto->send(sock, buff, len, flags);
    } else {
        if(sock->proto->type == SOCK_DGRAM || sock->proto->type == SOCK_RAW) {
            //FIXME errno = EDESTADDRREQ
        } else {
            //FIXME errno = ENOTCONN
        }
        return -1;
    }
}

uint32_t sock_recv(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    if(sock->flags & SOCK_FLAG_CONNECTED) {
        return sock->proto->recv(sock, buff, len, flags);
    } else {
        if(sock->proto->type == SOCK_DGRAM || sock->proto->type == SOCK_RAW) {
            //FIXME errno = EDESTADDRREQ
        } else {
            //FIXME errno = ENOTCONN
        }
        return -1;
    }
}

void sock_close(sock_t *sock) {
    sock->proto->close(sock);

    sock_free(sock);
}

static void sock_close_fd(gfd_t *gfd) {
    sock_close(gfd->private);
}

static fd_ops_t sock_ops = {
    .close = sock_close_fd,
};

gfd_idx_t sock_create_fd(uint32_t family, uint32_t type, uint32_t protocol) {
    sock_t *sock = sock_create(family, type, protocol);

    if(sock) {
        return gfdt_add(FD_SOCK, 0, &sock_ops, sock);
    } else {
        return FD_INVALID;
    }
}
