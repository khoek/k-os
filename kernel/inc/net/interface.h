#ifndef KERNEL_NET_INTERFACE_H
#define KERNEL_NET_INTERFACE_H

#include "lib/int.h"
#include "common/list.h"

#include "net/types.h"

typedef enum net_state {
    IF_DHCP,
    IF_UP,
    IF_ERROR
} net_state_t;

struct net_interface {
    list_head_t list;

    mac_t mac;
    ip_t ip;
    uint32_t rx_total, tx_total;
    
    net_state_t state;

    void (*rx_poll)(net_interface_t *);
    int32_t (*tx_send)(net_interface_t *, net_packet_t *);
};

void register_net_interface(net_interface_t *interface);
void unregister_net_interface(net_interface_t *interface);

#endif
