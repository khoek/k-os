#include <stddef.h>

#include "lib/int.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "bug/debug.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/packet.h"
#include "net/eth/eth.h"
#include "net/ip/ip.h"
#include "net/ip/arp.h"
#include "video/log.h"

static uint16_t type_lookup[] = {
    [PROTOCOL_IP]  = ETH_TYPE_IP ,
    [PROTOCOL_ARP] = ETH_TYPE_ARP
};

static void eth_resolve(packet_t *packet) {
    switch(packet->route.protocol) {
        case PROTOCOL_IP: {
            arp_resolve(packet);
            break;
        }
        default: BUG();
    }
}

static void eth_build_hdr(packet_t *packet) {
    eth_header_t *hdr = kmalloc(sizeof(eth_header_t));
    hdr->src = packet->route.hard.src->mac;
    hdr->dst = packet->route.hard.dst->mac;
    hdr->type = swap_uint16(type_lookup[packet->route.protocol] ? type_lookup[packet->route.protocol] : 0xFFFF);

    packet->link.buff = hdr;
    packet->link.size = sizeof(eth_header_t);
}

static void eth_recieve(packet_t *packet, void *raw, uint16_t length) {
    eth_header_t *header = packet->link.buff = raw;
    raw += sizeof(eth_header_t);
    length -= sizeof(eth_header_t);

    switch(swap_uint16(header->type)) {
        case ETH_TYPE_ARP: {
            arp_recv(packet, raw, length);
            break;
        }
        case ETH_TYPE_IP: {
            ip_recv(packet, raw, length);
            break;
        }
        default: {
            logf("eth - unrecognised ethertype (0x%02X)", swap_uint16(header->type));
            break;
        }
    }
}

net_link_layer_t eth_link_layer = {
    .resolve = eth_resolve,
    .build_hdr = eth_build_hdr,
    .recieve = eth_recieve,
};
