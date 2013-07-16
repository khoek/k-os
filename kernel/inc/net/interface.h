#ifndef KERNEL_NET_INTERFACE_H
#define KERNEL_NET_INTERFACE_H

#include "lib/int.h"
#include "common/list.h"
#include "common/listener.h"
#include "net/types.h"
#include "net/ip/ip.h"

typedef enum net_state {
    IF_DOWN,
    IF_UP,
    IF_READY,
    IF_ERROR
} net_state_t;

struct net_interface {
    list_head_t list;

    sock_addr_t hard_addr;

    ip_t ip;
    uint32_t rx_total, tx_total;

    net_state_t state;

    net_link_layer_t link_layer;

    int32_t (*send)(packet_t *);
};

void register_net_interface(net_interface_t *interface, net_state_t state);
void unregister_net_interface(net_interface_t *interface);

void register_net_state_listener(listener_t *listener);
void unregister_net_state_listener(listener_t *listener);

char * net_get_hostname();
void net_put_hostname();

void net_recieve(net_interface_t *interface, void *raw, uint16_t len);

void net_set_state(net_interface_t *interface, net_state_t state);

#endif
