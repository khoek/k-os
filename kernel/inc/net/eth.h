#ifndef KERNEL_NET_ETH
#define KERNEL_NET_ETH

#include "net/types.h"

#define ETH_TYPE_IP  0x0800
#define ETH_TYPE_ARP 0x0806

extern net_link_layer_t eth_link_layer;

#endif
