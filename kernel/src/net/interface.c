#include "lib/int.h"
#include "common/list.h"
#include "common/listener.h"
#include "sync/spinlock.h"
#include "net/packet.h"
#include "net/interface.h"
#include "video/log.h"

static char *hostname = "K-OS"; //TODO touppercase this when it gets dynamically loaded
static uint32_t hostname_handles;

static DEFINE_LIST(interfaces);
static DEFINE_LISTENER_CHAIN(listeners);
static DEFINE_SPINLOCK(interface_lock);
static DEFINE_SPINLOCK(listener_lock);

void register_net_interface(net_interface_t *interface) {
    interface->rx_total = 0;
    interface->tx_total = 0;

    interface->ip_data = 0;

    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    list_add(&interface->list, &interfaces);

    spin_unlock_irqstore(&interface_lock, flags);

    net_set_state(interface, IF_DOWN);
}

void unregister_net_interface(net_interface_t *interface) {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    list_rm(&interface->list);

    spin_unlock_irqstore(&interface_lock, flags);
}

void register_net_state_listener(listener_t *listener) {
    uint32_t flags;
    spin_lock_irqsave(&listener_lock, &flags);

    listener_chain_add(listener, &listeners);

    spin_unlock_irqstore(&listener_lock, flags);
}

void unregister_net_state_listener(listener_t *listener) {
    uint32_t flags;
    spin_lock_irqsave(&listener_lock, &flags);

    listener_chain_rm(listener);

    spin_unlock_irqstore(&listener_lock, flags);
}

const char * net_get_hostname() {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    hostname_handles++;
    char *local_name = ACCESS_ONCE(hostname);

    spin_unlock_irqstore(&interface_lock, flags);

    return local_name;
}

void net_put_hostname() {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    hostname_handles--;

    spin_unlock_irqstore(&interface_lock, flags);
}

void net_recieve(net_interface_t *interface, void *raw, uint16_t len) {
    packet_t packet;
    packet.interface = interface;
    interface->link_layer.handle(&packet, raw, len);
}

void net_set_state(net_interface_t *interface, net_state_t state) {
    if(interface->state == state) return;

    //FIXME locking
    interface->state = state;

    //TODO better handle active devices, etc
    switch(state) {
        case IF_DOWN: {
            logf("net - interface is DOWN");
            break;
        }
        case IF_UP: {
            logf("net - interface is UP");
            break;
        }
        case IF_READY: {
            logf("net - interface is READY");
            break;
        }
        case IF_ERROR: {
            logf("net - interface error");
            break;
        }
        default: break;
    }

    uint32_t flags;
    spin_lock_irqsave(&listener_lock, &flags);

    listener_chain_fire(state, interface, &listeners);

    spin_unlock_irqstore(&listener_lock, flags);
}

net_interface_t * net_primary_interface() {
    return list_first(&interfaces, net_interface_t, list);
}
