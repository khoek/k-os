#include "lib/int.h"
#include "lib/string.h"
#include "common/swap.h"
#include "common/hashtable.h"
#include "sync/spinlock.h"
#include "mm/cache.h"
#include "net/packet.h"
#include "net/interface.h"
#include "net/eth/eth.h"
#include "net/ip/arp.h"
#include "video/log.h"

typedef enum arp_cache_entry_state {
    CACHE_UNRESOLVED,
    CACHE_RESOLVED
} arp_cache_entry_state_t;

typedef struct arp_cache_entry {
    hashtable_node_t node;

    ip_t ip;
    mac_t mac;

    arp_cache_entry_state_t state;

    spinlock_t lock;
    list_head_t pending;
} arp_cache_entry_t;

static DEFINE_HASHTABLE(arp_cache, 5);
static DEFINE_SPINLOCK(arp_cache_lock);

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

    packet->net.buff = hdr;
    packet->net.size = sizeof(arp_header_t);

    packet->state = P_RESOLVED;

    packet->route.protocol = ETH_TYPE_ARP;
    packet->route.src.family = AF_LINK;
    packet->route.src.addr = &hdr->sender_mac;
    packet->route.dst.family = AF_LINK;
    packet->route.dst.addr = (op == ARP_OP_REQUEST) ? (mac_t *) &MAC_BROADCAST : &hdr->target_mac;
}

static arp_cache_entry_t * arp_cache_find(ip_t *ip) {
    arp_cache_entry_t *entry;
    HASHTABLE_FOR_EACH_COLLISION(*((uint32_t *) ip), entry, arp_cache, node) {
        if(!memcmp(ip, &entry->ip, sizeof(ip_t))) return entry;
    }

    return NULL;
}

static void arp_cache_put_resolved(ip_t ip, mac_t mac) {
    arp_cache_entry_t *entry = kmalloc(sizeof(arp_cache_entry_t));
    entry->state = CACHE_RESOLVED;
    entry->ip = ip;
    entry->mac = mac;

    spinlock_init(&entry->lock);

    list_init(&entry->pending);

    hashtable_add(*((uint32_t *) ip.addr), &entry->node, arp_cache);
}

static void arp_cache_put_unresolved(ip_t ip, packet_t *packet) {
    arp_cache_entry_t *entry = kmalloc(sizeof(arp_cache_entry_t));
    entry->state = CACHE_UNRESOLVED;
    entry->ip = ip;

    spinlock_init(&entry->lock);

    list_init(&entry->pending);
    list_add(&packet->list, &entry->pending);

    hashtable_add(*((uint32_t *) ip.addr), &entry->node, arp_cache);

    packet_t *request = packet_create(packet->interface, NULL, 0);
    arp_build(request, ARP_OP_REQUEST, *((mac_t *) packet->interface->hard_addr.addr), MAC_NONE, ((ip_interface_t *) packet->interface->ip_data)->ip_addr, ip);
    packet_send(request);
}

void arp_resolve(packet_t *packet) {
    ip_t ip = *((ip_t *) packet->route.dst.addr);

    packet->route.src = packet->interface->hard_addr;

    if(!memcmp(&ip, &IP_BROADCAST, sizeof(ip_t))) {
        packet->route.dst.family = AF_LINK;
        packet->route.dst.addr = (mac_t *) &MAC_BROADCAST;

        packet->state = P_RESOLVED;
        packet_send(packet);
    } else {
        uint32_t flags;
        spin_lock_irqsave(&arp_cache_lock, &flags);

        arp_cache_entry_t *entry = arp_cache_find(&ip);

        if(entry) {
            uint32_t flags;
            spin_lock_irqsave(&entry->lock, &flags);

            if(entry->state == CACHE_UNRESOLVED) {
                list_add(&packet->list, &entry->pending);
            } else if(entry->state == CACHE_RESOLVED) {

                packet->route.dst.family = AF_LINK;
                packet->route.dst.addr = &entry->mac;

                packet->state = P_RESOLVED;
                packet_send(packet);
            }

            spin_unlock_irqstore(&entry->lock, flags);
        } else {
            arp_cache_put_unresolved(ip, packet);
        }

        spin_unlock_irqstore(&arp_cache_lock, flags);
    }
}

void arp_recv(packet_t *packet, void *raw, uint16_t len) {
    arp_header_t *arp = packet->net.buff = raw;
    if(!memcmp(&((ip_interface_t *) packet->interface->ip_data)->ip_addr.addr, &arp->target_ip.addr, sizeof(ip_t))) {
        if(!memcmp(&arp->target_mac, &MAC_NONE, sizeof(mac_t))) {
            packet_t *response = packet_create(packet->interface, NULL, 0);
            arp_build(response, ARP_OP_RESPONSE, *((mac_t *) packet->interface->hard_addr.addr), arp->sender_mac, ((ip_interface_t *) packet->interface->ip_data)->ip_addr, arp->sender_ip);
            packet_send(response);
        }
    }

    uint32_t flags;
    spin_lock_irqsave(&arp_cache_lock, &flags);

    arp_cache_entry_t *entry = arp_cache_find(&arp->sender_ip);
    if(entry) {
        if(entry->state == CACHE_UNRESOLVED) {
            uint32_t flags;
            spin_lock_irqsave(&entry->lock, &flags);

            entry->state = CACHE_RESOLVED;
            entry->mac = arp->sender_mac;

            spin_unlock_irqstore(&entry->lock, flags);

            packet_t *pending;
            LIST_FOR_EACH_ENTRY(pending, &entry->pending, list) {
                pending->route.dst.family = AF_LINK;
                pending->route.dst.addr = &entry->mac;

                pending->state = P_RESOLVED;
                packet_send(pending);
            }
        }
    } else {
        arp_cache_put_resolved(arp->sender_ip, arp->sender_mac);
    }

    spin_unlock_irqstore(&arp_cache_lock, flags);
}
