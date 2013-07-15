#include "lib/int.h"
#include "lib/string.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "net/socket.h"

static sock_family_t *families[PF_MAX];
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

    sock_t *sock = sock_alloc();
    sock->family = families[family];
    sock->proto = families[family]->find(type, protocol);
    sock->proto->open(sock);

    spin_unlock_irqstore(&family_lock, flags);

    return sock;
}

void sock_close(sock_t *sock) {
    sock->proto->close(sock);

    sock_free(sock);
}
