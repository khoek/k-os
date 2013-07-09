#include "lib/int.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "video/log.h"

void layer_link_eth(packet_t *raw, uint16_t type, mac_t src, mac_t dst) {
    ethernet_header_t *hdr = kmalloc(sizeof(ethernet_header_t));
    hdr->src = src;
    hdr->dst = dst;
    hdr->type = swap_uint16(type);

    raw->link.eth = hdr;
    raw->link_size = sizeof(ethernet_header_t);
}

void recv_link_eth(net_interface_t *interface, void *raw, uint16_t length) {
    packet_t packet;

    ethernet_header_t *header = packet.link.eth = raw;
    raw += sizeof(ethernet_header_t);
    length -= sizeof(ethernet_header_t);

    header->type = swap_uint16(header->type);
    switch(header->type) {
        case ETH_TYPE_ARP: {
            recv_net_arp(interface, &packet, raw, length);
            break;
        }
        case ETH_TYPE_IP: {
            recv_net_ip(interface, &packet, raw, length);
            break;
        }
        default: {
            logf("ethernet - unrecognised raw type (0x%02X)", header->type);
            break;
        }
    }
}
