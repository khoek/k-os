#include <stddef.h>

#include "lib/int.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "bug/debug.h"
#include "mm/cache.h"
#include "net/packet.h"
#include "net/socket.h"
#include "net/eth/eth.h"
#include "net/ip/ip.h"
#include "net/ip/arp.h"
#include "video/log.h"

static void eth_resolve(packet_t *packet) {
    switch(packet->route.dst.family) {
        case AF_INET: {
            arp_resolve(packet);
            break;
        }
        default: BUG();
    }
}

static void eth_build_hdr(packet_t *packet) {
    BUG_ON(packet->route.src.family != AF_LINK);
    BUG_ON(packet->route.dst.family != AF_LINK);

    eth_header_t *hdr = kmalloc(sizeof(eth_header_t));
    hdr->src = *((mac_t *) packet->route.src.addr);
    hdr->dst = *((mac_t *) packet->route.dst.addr);
    hdr->type = swap_uint16(packet->route.protocol);

    packet->link.buff = hdr;
    packet->link.size = sizeof(eth_header_t);
}

static void eth_handle(packet_t *packet, void *raw, uint16_t length) {
    eth_header_t *header = packet->link.buff = raw;
    raw += sizeof(eth_header_t);
    length -= sizeof(eth_header_t);

    switch(swap_uint16(header->type)) {
        case ETH_TYPE_ARP: {
            arp_handle(packet, raw, length);
            break;
        }
        case ETH_TYPE_IP: {
            ip_handle(packet, raw, length);
            break;
        }
        default: {
            kprintf("eth - unrecognised ethertype (0x%02X)", swap_uint16(header->type));
            break;
        }
    }
}

net_link_layer_t eth_link_layer = {
    .resolve = eth_resolve,
    .build_hdr = eth_build_hdr,
    .handle = eth_handle,
};
