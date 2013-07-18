#include "lib/int.h"
#include "lib/string.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "net/socket.h"
#include "fs/fd.h"

static sock_family_t *families[AF_MAX];
static DEFINE_SPINLOCK(family_lock);

void register_sock_family(sock_family_t *family) {
    uint32_t flags;
    spin_lock_irqsave(&family_lock, &flags);

    if(families[family->code]) panicf("Socket family %u already registered!", family->code);
    families[family->code] = family;

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
