#ifndef KERNEL_NET_H
#define KERNEL_NET_H

#include "int.h"
#include "list.h"

typedef struct net_interface net_interface_t;

struct net_interface {
    list_head_t list;

    uint8_t mac[6];

    uint32_t rx_total, tx_total;

    void (*rx_poll)(net_interface_t *);
    int32_t (*tx_send)(net_interface_t *, void *, uint16_t);
};

void register_net_interface(net_interface_t *interface);
void unregister_net_interface(net_interface_t *interface);

#endif
