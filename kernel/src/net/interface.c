#include "lib/int.h"
#include "common/list.h"
#include "atomic/spinlock.h"
#include "net/types.h"
#include "net/dhcp.h"
#include "video/log.h"

static LIST_HEAD(interfaces);
static SPINLOCK_INIT(interface_lock);

void register_net_interface(net_interface_t *interface) {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    interface->ip = IP_NONE;

    interface->rx_total = 0;
    interface->tx_total = 0;

    interface->state = IF_INIT;

    list_add(&interface->list, &interfaces);

    spin_unlock_irqstore(&interface_lock, flags);

    logf("net - interface registerd with MAC %X:%X:%X:%X:%X:%X",
        interface->mac.addr[0],
        interface->mac.addr[1],
        interface->mac.addr[2],
        interface->mac.addr[3],
        interface->mac.addr[4],
        interface->mac.addr[5]
    );

    dhcp_start(interface);
}

void unregister_net_interface(net_interface_t *interface) {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    list_rm(&interface->list);

    spin_unlock_irqstore(&interface_lock, flags);
}

void net_set_state(net_interface_t *interface, net_state_t state) {
    //FIXME locking
    interface->state = state;

    //TODO better handle active devices, etc
    switch(state) {
        case IF_READY: {
            logf("net - interface is READY: %u.%u.%u.%u",
                interface->ip.addr[0], interface->ip.addr[1],
                interface->ip.addr[2], interface->ip.addr[3]
            );
            break;
        }
        case IF_DHCP: {
            logf("net - interface commencing DHCP");
            break;
        }
        case IF_ERROR: {
            logf("net - interface error");
            break;
        }
        default: break;
    }
}
