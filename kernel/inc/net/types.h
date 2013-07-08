#ifndef KERNEL_NET_TYPES_H
#define KERNEL_NET_TYPES_H

#include "lib/int.h"

typedef struct mac { uint8_t addr[6]; } mac_t;
typedef struct ip { uint8_t addr[4]; } ip_t;

#define MAC_BROADCAST ((mac_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} })

#define IP_BROADCAST ((ip_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF} })
#define IP_NONE ((ip_t) { .addr = {0x00, 0x00, 0x00, 0x00} })

typedef struct net_packet {
    void *link_hdr;
    uint32_t link_len;
    void *net_hdr;
    uint32_t net_len;
    void *tran_hdr;
    uint32_t tran_len;
    void *payload;
    uint32_t payload_len;
} net_packet_t;

typedef struct net_interface net_interface_t;

#endif
