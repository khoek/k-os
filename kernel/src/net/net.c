#include "list.h"
#include "net.h"
#include "spinlock.h"
#include "log.h"

static LIST_HEAD(interfaces);
static SPINLOCK_INIT(interface_lock);

void register_net_interface(net_interface_t *interface) {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    interface->rx_total = 0;
    interface->tx_total = 0;

    list_add(&interface->list, &interfaces);

    spin_unlock_irqstore(&interface_lock, flags);

    logf("net - interface registerd with MAC %X:%X:%X:%X:%X:%X",
        interface->mac[0],
        interface->mac[1],
        interface->mac[2],
        interface->mac[3],
        interface->mac[4],
        interface->mac[5]
    );
}

void unregister_net_interface(net_interface_t *interface) {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    list_rm(&interface->list);

    spin_unlock_irqstore(&interface_lock, flags);
}
