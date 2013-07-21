#ifndef KERNEL_NET_ETH_ETH_H
#define KERNEL_NET_ETH_ETH_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/socket.h"

typedef struct mac { uint8_t addr[6]; } mac_t;

static const mac_t MAC_BROADCAST = { .addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} };
static const mac_t MAC_NONE = { .addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

#define ETH_MIN_PACKET_SIZE 60

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