#ifndef KERNEL_NET_ETH_ETH_H
#define KERNEL_NET_ETH_ETH_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/types.h"

#define HTYPE_ETH  1

#define ETH_TYPE_IP  0x0800
#define ETH_TYPE_ARP 0x0806

typedef struct eth_header {
    mac_t dst;
    mac_t src;
    uint16_t type;
} PACKED eth_header_t;

extern net_link_layer_t eth_link_layer;

#endif
