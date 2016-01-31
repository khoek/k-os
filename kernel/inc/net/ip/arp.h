#ifndef KERNEL_NET_IP_ARP_H
#define KERNEL_NET_IP_ARP_H

#include "common/types.h"
#include "common/compiler.h"
#include "net/interface.h"
#include "net/eth/eth.h"
#include "net/ip/ip.h"

#define ARP_OP_REQUEST  0x0001
#define ARP_OP_RESPONSE 0x0002

typedef struct arp_header {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t op;
    mac_t sender_mac;
    ip_t sender_ip;
    mac_t target_mac;
    ip_t target_ip;
} PACKED arp_header_t;

void arp_build(packet_t *packet, uint16_t op, mac_t sender_mac, mac_t target_mac, ip_t sender_ip, ip_t target_ip);
void arp_handle(packet_t *packet, void *raw, uint16_t len);
void arp_resolve(packet_t *packet);

void arp_cache_store(net_interface_t *interface, mac_t *mac, ip_t *ip);

#endif
