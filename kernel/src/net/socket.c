#include "lib/int.h"
#include "lib/string.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "net/socket.h"
#include "fs/fd.h"
#include "fs/vfs.h"

#define ISCONNECTIONLESS(sock) (sock->proto->type == SOCK_DGRAM || sock->proto->type == SOCK_RAW)

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
    sock_family_t *family_ptr;
    sock_t *sock = NULL;

    uint32_t flags;
    spin_lock_irqsave(&family_lock, &flags);

    if(!(family_ptr = families[family])) {
        return NULL;
    }

    sock_protocol_t *proto = family_ptr->find(type, protocol);

    spin_unlock_irqstore(&family_lock, flags);

    if(proto) {
        sock = sock_open(family_ptr, proto);
    }

    return sock;
}

sock_t * sock_open(sock_family_t *family, sock_protocol_t *proto) {
    uint32_t flags;
    spin_lock_irqsave(&family_lock, &flags);

    sock_t *sock = sock_alloc();
    sock->family = family;
    sock->proto = proto;
    proto->open(sock);

    spin_unlock_irqstore(&family_lock, flags);

    return sock;
}

bool sock_listen(sock_t *sock, uint32_t backlog) {
    if(sock->flags & SOCK_FLAG_CONNECTED) {
        //FIXME errno = EINVAL
        return false;
    }

    return sock->proto->listen(sock, backlog);
}

sock_t * sock_accept(sock_t *sock) {
    if(!(sock->flags & SOCK_FLAG_LISTENING)) {
        //FIXME errno = EINVAL
        return false;
    }

    return sock->proto->accept(sock);
}

bool sock_bind(sock_t *sock, sock_addr_t *addr) {
    if((sock->flags & SOCK_FLAG_SHUT_RDWR) == SOCK_FLAG_SHUT_RDWR) {
        //FIXME errno = EINVAL
        return false;
    }

    if(sock->flags & SOCK_FLAG_BOUND) {
        //FIXME errno = EINVAL
        return false;
    }

    if(sock->flags & SOCK_FLAG_LISTENING) {
        //FIXME errno = EINVAL
        return false;
    }

    if(sock->flags & SOCK_FLAG_CONNECTED) {
        //FIXME errno = EISCONN
        return false;
    }

    return sock->proto->bind(sock, addr);
}

bool sock_connect(sock_t *sock, sock_addr_t *addr) {
    if(!ISCONNECTIONLESS(sock)) {
        if(sock->flags & SOCK_FLAG_CONNECTED) {
            //FIXME errno = EISCONN
            return false;
        }
    }

    return sock->proto->connect(sock, addr);
}

bool sock_shutdown(sock_t *sock, int how) {
    if(ISCONNECTIONLESS(sock) || !(sock->flags & SOCK_FLAG_CONNECTED)) {
        //FIXME errno = ENOTCONN
        return false;
    }

    if(how & (~SHUT_MASK)) {
        //FIXME errno = EINVAL
        return false;
    }

    int bits = 0;

    switch(how) {
        case SHUT_RD:
            bits |= SOCK_FLAG_SHUT_RD;
            break;
        case SHUT_WR:
            bits |= SOCK_FLAG_SHUT_WR;
            break;
        case SHUT_RDWR:
            bits |= SOCK_FLAG_SHUT_RD;
            bits |= SOCK_FLAG_SHUT_WR;
            break;
    }

    if(!bits) return true;

    //ignore bits which have already been set
    if(sock->flags & SOCK_FLAG_SHUT_RD) bits &= ~SOCK_FLAG_SHUT_RD;
    if(sock->flags & SOCK_FLAG_SHUT_WR) bits &= ~SOCK_FLAG_SHUT_WR;

    return sock->proto->shutdown(sock, bits);
}

uint32_t sock_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    if(sock->flags & SOCK_FLAG_CONNECTED) {
        return sock->proto->send(sock, buff, len, flags);
    } else {
        if(ISCONNECTIONLESS(sock)) {
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
        if(ISCONNECTIONLESS(sock)) {
            //FIXME errno = EDESTADDRREQ
        } else {
            //FIXME errno = ENOTCONN
        }
        return -1;
    }
}

void sock_close(sock_t *sock) {
    if(!(sock->flags & SOCK_FLAG_CLOSED)) {
        sock->flags |= SOCK_FLAG_CLOSED;

        sock->proto->close(sock);
    }

    sock_free(sock);
}

static void sock_close_fd(file_t *file) {
    if(((sock_t *) file->private)->flags & SOCK_FLAG_CLOSED) {
        return;
    }

    sock_close(file->private);
}

static file_ops_t sock_ops = {
    .close = sock_close_fd,
};

gfd_idx_t sock_create_fd(sock_t *sock) {
    file_t *file = file_alloc(&sock_ops);
    file->private = sock;

    return gfdt_add(file);
}
