#include "lib/int.h"
#include "lib/string.h"
#include "common/swap.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "net/interface.h"
#include "video/log.h"

void layer_net_arp(packet_t *packet, uint16_t op, mac_t sender_mac, mac_t target_mac, ip_t sender_ip, ip_t target_ip) {
    arp_header_t *hdr = kmalloc(sizeof(arp_header_t));

    hdr->htype = swap_uint16(HTYPE_ETH);
    hdr->ptype = swap_uint16(ETH_TYPE_IP);
    hdr->hlen = sizeof(mac_t);
    hdr->plen = sizeof(ip_t);
    hdr->op = swap_uint16(op);
    hdr->sender_mac = sender_mac;
    hdr->sender_ip = sender_ip;
    hdr->target_mac = target_mac;
    hdr->target_ip = target_ip;

    packet->net.arp = hdr;
    packet->net_size = sizeof(arp_header_t);

    layer_link_eth(packet, ETH_TYPE_ARP, sender_mac, target_mac);
}

void recv_net_arp(net_interface_t *interface, void *packet, uint16_t len) {
    arp_header_t *arp = (arp_header_t *) packet;

    if(!memcmp(&interface->ip.addr, &arp->target_ip.addr, sizeof(ip_t))) {
        packet_t *response = packet_alloc(NULL, 0);
        layer_net_arp(response, ARP_OP_RESPONSE, interface->mac, arp->sender_mac, interface->ip, arp->sender_ip);
        packet_send(interface, response);
        packet_free(response);
    }
}
