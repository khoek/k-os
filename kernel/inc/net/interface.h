#ifndef KERNEL_NET_INTERFACE_H
#define KERNEL_NET_INTERFACE_H

#include "lib/int.h"
#include "common/list.h"

#include "net/types.h"

typedef enum net_state {
    IF_DOWN,
    IF_UP,
    IF_DHCP,
    IF_READY,
    IF_ERROR
} net_state_t;

struct net_interface {
    list_head_t list;

    mac_t mac;
    ip_t ip;
    uint32_t rx_total, tx_total;

    net_state_t state;

    net_link_layer_t hard;

    int32_t (*tx_send)(net_interface_t *, packet_t *);
};

void register_net_interface(net_interface_t *interface, net_state_t state);
void unregister_net_interface(net_interface_t *interface);

char * net_get_hostname();
void net_put_hostname();

void net_recieve(net_interface_t *interface, void *raw, uint16_t len);

void net_set_state(net_interface_t *interface, net_state_t state);

#endif
