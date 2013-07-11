#include "lib/int.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "video/log.h"

void eth_build(packet_t *packet, uint16_t type, mac_t src, mac_t dst) {
    ethernet_header_t *hdr = kmalloc(sizeof(ethernet_header_t));
    hdr->src = src;
    hdr->dst = dst;
    hdr->type = swap_uint16(type);

    packet->link.eth = hdr;
    packet->link_size = sizeof(ethernet_header_t);
}

void eth_recv(net_interface_t *interface, void *raw, uint16_t length) {
    packet_t packet;
    packet.interface = interface;

    ethernet_header_t *header = packet.link.eth = raw;
    raw += sizeof(ethernet_header_t);
    length -= sizeof(ethernet_header_t);

    header->type = swap_uint16(header->type);
    switch(header->type) {
        case ETH_TYPE_ARP: {
            arp_recv(&packet, raw, length);
            break;
        }
        case ETH_TYPE_IP: {
            ip_recv(&packet, raw, length);
            break;
        }
        default: {
            logf("ethernet - unrecognised ethertype (0x%02X)", header->type);
            break;
        }
    }
}
