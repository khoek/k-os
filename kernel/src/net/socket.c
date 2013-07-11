#include "lib/int.h"
#include "common/hashtable.h"
#include "sync/spinlock.h"
#include "net/socket.h"

static DEFINE_HASHTABLE(types, 4);
static DEFINE_SPINLOCK(type_lock);

void register_sock_type(sock_type_t *type) {
    uint32_t flags;
    spin_lock_irqsave(&type_lock, &flags);

    hashtable_add(((((uint64_t) type->family) << 32) | type->type), &type->node, types);

    spin_unlock_irqstore(&type_lock, flags);
}

void unregister_sock_type(sock_type_t *type) {
    uint32_t flags;
    spin_lock_irqsave(&type_lock, &flags);

    hashtable_rm(&type->node);

    spin_unlock_irqstore(&type_lock, flags);
}
