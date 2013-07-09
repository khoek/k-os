#include "lib/int.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "video/log.h"

#include "eth.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

typedef struct ethernet_header {
    mac_t dst;
    mac_t src;
    uint16_t type;
} PACKED ethernet_header_t;

void layer_link_eth(net_packet_t *packet, uint16_t type, mac_t src, mac_t dst) {
    ethernet_header_t *hdr = kmalloc(sizeof(ethernet_header_t));
    hdr->src = src;
    hdr->dst = dst;
    hdr->type = type;

    packet->link_hdr = hdr;
    packet->link_len = sizeof(ethernet_header_t);
}

void recv_link_eth(net_interface_t *interface, void *packet, uint16_t length) {
    ethernet_header_t *header = (ethernet_header_t *) packet;
    packet += sizeof(ethernet_header_t);
    length -= sizeof(ethernet_header_t);

    switch(header->type) {
        case ETH_TYPE_IP: {
            recv_net_ip(interface, packet, length);
            break;
        }
        case ETH_TYPE_ARP: {
            recv_net_arp(interface, packet, length);
            break;
        }
        default: {
            logf("ethernet - unrecognised packet type (0x%02X)", header->type);
            break;
        }
    }
}
