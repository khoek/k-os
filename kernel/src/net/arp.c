#include "lib/int.h"
#include "lib/string.h"
#include "common/swap.h"
#include "common/hashtable.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "net/interface.h"
#include "net/eth.h"
#include "net/arp.h"
#include "video/log.h"

typedef struct arp_cache_entry {
    hlist_node_t node;

    ip_t ip;
    mac_t mac;
} arp_cache_entry_t;

static DEFINE_HASHTABLE(arp_lookup, 5);

void arp_build(packet_t *packet, uint16_t op, mac_t sender_mac, mac_t target_mac, ip_t sender_ip, ip_t target_ip) {
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

    packet->state = P_RESOLVED;

    packet->route.protocol = PROTOCOL_ARP;
    packet->route.hard.src = (hard_addr_t *) &hdr->sender_mac;
    packet->route.hard.dst = (hard_addr_t *) &hdr->target_mac;
}

void arp_resolve(packet_t *packet) {
    ip_t ip = *((ip_t *) packet->route.sock.dst);

    packet->route.hard.src = (hard_addr_t *) &packet->interface->mac;

    if(!memcmp(&ip, &IP_BROADCAST, sizeof(ip_t))) {
        packet->route.hard.dst = (hard_addr_t *) &MAC_BROADCAST;
    } else {
        //FIXME do an ARP lookup
    }

    packet->state = P_RESOLVED;

    packet_send(packet);
}

static void arp_cache(ip_t ip, mac_t mac) {
    arp_cache_entry_t *entry = kmalloc(sizeof(arp_cache_entry_t));
    entry->ip = ip;
    entry->mac = mac;

    hashtable_add(*((uint32_t *) ip.addr), &entry->node, arp_lookup);
}

void arp_recv(packet_t *packet, void *raw, uint16_t len) {
    arp_header_t *arp = packet->net.arp = raw;
    if(!memcmp(&packet->interface->ip.addr, &arp->target_ip.addr, sizeof(ip_t))) {
        if(!memcmp(&packet->interface->mac.addr, &MAC_NONE, sizeof(ip_t))) {
            packet_t *response = packet_create(packet->interface, NULL, 0);
            arp_build(response, ARP_OP_RESPONSE, packet->interface->mac, arp->sender_mac, packet->interface->ip, arp->sender_ip);
            packet_send(response);
        }
    }

    arp_cache(arp->sender_ip, arp->sender_mac);
}
