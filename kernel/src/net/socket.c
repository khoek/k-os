#include "lib/int.h"
#include "lib/string.h"
#include "bug/panic.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "net/socket.h"

static socket_family_t *families[PF_MAX];
static DEFINE_SPINLOCK(family_lock);

void register_socket_family(socket_family_t *family) {
    uint32_t flags;
    spin_lock_irqsave(&family_lock, &flags);

    if(!families[family->code]) panicf("Socket family %u already registered!", family->code);
    families[family->code] = family;

    spin_unlock_irqstore(&family_lock, flags);
}

static socket_t * socket_alloc() {
    socket_t *alloc = kmalloc(sizeof(socket_t));
    memset(alloc, 0, sizeof(socket_t));

    return alloc;
}

static void socket_free(socket_t *sock) {
    kfree(sock, sizeof(socket_t));
}

socket_t * socket_create(uint32_t family, uint32_t type, uint32_t protocol) {
    uint32_t flags;
    spin_lock_irqsave(&family_lock, &flags);

    if(!families[family]) {
        return NULL;
    }

    socket_t *sock = socket_alloc();
    sock->proto = families[family]->create(type, protocol);
    sock->proto->create(sock);

    spin_unlock_irqstore(&family_lock, flags);

    return sock;
}

void socket_destroy(socket_t *sock) {
    sock->proto->destroy(sock);

    socket_free(sock);
}
