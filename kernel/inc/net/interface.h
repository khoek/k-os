#ifndef KERNEL_NET_INTERFACE_H
#define KERNEL_NET_INTERFACE_H

#include "lib/int.h"
#include "common/list.h"
#include "common/listener.h"
#include "net/packet.h"

typedef struct net_link_layer {
    void (*resolve)(packet_t *packet);
    void (*build_hdr)(packet_t *);
    void (*handle)(packet_t *, void *, uint16_t);
} net_link_layer_t;

typedef enum net_state {
    IF_UNKNOWN=0,
    IF_INIT,
    IF_DOWN,
    IF_UP,
    IF_READY,
    IF_ERROR
} net_state_t;

typedef struct net_interface {
    list_head_t list;

    net_state_t state;

    void *ip_data;

    sock_addr_t hard_addr;

    uint32_t rx_total, tx_total;

    net_link_layer_t link_layer;

    int32_t (*send)(packet_t *);
} net_interface_t;

void register_net_interface(net_interface_t *interface);
void unregister_net_interface(net_interface_t *interface);

void register_net_state_listener(listener_t *listener);
void unregister_net_state_listener(listener_t *listener);

const char * net_get_hostname();
void net_put_hostname();

void net_recieve(net_interface_t *interface, void *raw, uint16_t len);

void net_set_state(net_interface_t *interface, net_state_t state);

net_interface_t * net_primary_interface();

#endif
