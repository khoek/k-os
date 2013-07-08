#include "lib/string.h"
#include "common/list.h"
#include "common/swap.h"
#include "common/init.h"
#include "common/compiler.h"
#include "atomic/spinlock.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/dhcp.h"
#include "video/log.h"

static LIST_HEAD(interfaces);
static SPINLOCK_INIT(interface_lock);

typedef struct ethernet_header {
    mac_t dst;
    mac_t src;
    uint16_t type;
} PACKED ethernet_header_t;

#define IP_VERSION(x)   ((x & 0xF0) >> 4)
#define IP_IHL(x)       ((x & 0x0F))
#define IP_DSCP(x)      ((x & 0xFC) >> 2)
#define IP_ECN(x)       ((x & 0x03))
#define IP_FLAGS(x)     ((x & 0x00E0) >> 5)
#define IP_FRAG_OFF(x)  ((x & 0xFF1F))

#define IP_FLAG_EVIL       (1 << 5) //RFC 3514 :D
#define IP_FLAG_DONT_FRAG  (1 << 6)
#define IP_FLAG_MORE_FRAGS (1 << 7)

#define IP(version) (version)

typedef struct ip_header {
    uint8_t  version_ihl;
    uint8_t  dscp_ecn;
    uint16_t total_length;
    uint16_t ident;
    uint16_t flags_frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    ip_t src;
    ip_t dst;
} PACKED ip_header_t;

#define TCP_DATA_OFF(x) ((x & 0xF000) >> 4)
#define TCP_FLAGS(x)    ((x & 0x0FFF) >> 4)

typedef struct tcp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t data_off_flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent;
} PACKED tcp_header_t;

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} PACKED udp_header_t;

void register_net_interface(net_interface_t *interface) {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    interface->ip = IP_NONE;
    
    interface->rx_total = 0;
    interface->tx_total = 0;
    
    interface->state = IF_DHCP;

    list_add(&interface->list, &interfaces);

    spin_unlock_irqstore(&interface_lock, flags);

    logf("net - interface registerd with MAC %X:%X:%X:%X:%X:%X",
        interface->mac.addr[0],
        interface->mac.addr[1],
        interface->mac.addr[2],
        interface->mac.addr[3],
        interface->mac.addr[4],
        interface->mac.addr[5]
    );
    
    dhcp_start(interface);
}

void unregister_net_interface(net_interface_t *interface) {
    uint32_t flags;
    spin_lock_irqsave(&interface_lock, &flags);

    list_rm(&interface->list);

    spin_unlock_irqstore(&interface_lock, flags);
}

void recv_link_eth(net_interface_t *interface, void *packet, uint16_t length) {
    ethernet_header_t *header = (ethernet_header_t *) packet;
    packet += sizeof(ethernet_header_t);
    length -= sizeof(ethernet_header_t);

    switch(header->type) {
        case ETH_TYPE_IP: {
            ip_header_t *ip = (ip_header_t *) packet;
            packet += sizeof(ip_header_t);
            length -= sizeof(ip_header_t);

            if(IP_VERSION(ip->version_ihl) != 0x04) {
                logf("ethernet - ip packet with unsupported version number (0x%02X)", IP_VERSION(ip->version_ihl));
            } else {
                switch(ip->protocol) {
                    case IP_PROT_TCP: {
                        tcp_header_t *tcp = (tcp_header_t *) packet;
                        packet += sizeof(tcp_header_t);
                        length -= sizeof(tcp_header_t);

                        logf("ethernet - tcp packet detected");
                        logf("ethernet - src %u.%u.%u.%u port %u dst %u.%u.%u.%u port %u",
                            ip->src.addr[0], ip->src.addr[1], ip->src.addr[2], ip->src.addr[3], swap_uint16(tcp->src_port),
                            ip->dst.addr[0], ip->dst.addr[1], ip->dst.addr[2], ip->dst.addr[3], swap_uint16(tcp->dst_port)
                        );

                        break;
                    }
                    case IP_PROT_UDP: {
                        udp_header_t *udp = (udp_header_t *) packet;
                        packet += sizeof(udp_header_t);
                        length -= sizeof(udp_header_t);

                        logf("ethernet - udp packet detected");
                        logf("ethernet - src %u.%u.%u.%u port %u dst %u.%u.%u.%u port %u",
                            ip->src.addr[0], ip->src.addr[1], ip->src.addr[2], ip->src.addr[3], swap_uint16(udp->src_port),
                            ip->dst.addr[0], ip->dst.addr[1], ip->dst.addr[2], ip->dst.addr[3], swap_uint16(udp->dst_port)
                        );
                        break;
                    }
                    default: {
                        logf("ethernet - ip packet with urecognised protocol (0x%02X)", ip->protocol);
                        break;
                    }
                }
            }

            break;
        }
        case ETH_TYPE_ARP: {
            logf("ethernet - arp packet detected");
            break;
        }
        default: {
            logf("ethernet - unrecognised packet type (0x%02X)", header->type);
            break;
        }
    }
}

net_packet_t * packet_alloc(void *payload, uint16_t len) {
    net_packet_t *packet = kmalloc(sizeof(net_packet_t));
    memset(packet, 0, sizeof(net_packet_t));
    
    packet->payload = payload;
    packet->payload_len = len;

    return packet;
}

void packet_free(net_packet_t *packet) {
    kfree(packet, sizeof(net_packet_t));
}

void packet_send(net_interface_t *interface, net_packet_t *packet) {    
    interface->tx_send(interface, packet);
    
    if(packet->link_hdr) kfree(packet->link_hdr, packet->link_len);
    if(packet->net_hdr) kfree(packet->net_hdr, packet->net_len);
    if(packet->tran_hdr) kfree(packet->tran_hdr, packet->tran_len);
}

void layer_link_eth(net_packet_t *packet, uint16_t type, mac_t src, mac_t dst) {
    ethernet_header_t *hdr = kmalloc(sizeof(ethernet_header_t));
    hdr->src = src;
    hdr->dst = dst;
    hdr->type = type;
 
    packet->link_hdr = hdr;
    packet->link_len = sizeof(ethernet_header_t);
}

static uint16_t sum_to_checksum(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }
    
    return ~sum;
}

void layer_net_ip(net_packet_t *packet, uint8_t protocol, ip_t src, ip_t dst) {
    ip_header_t *hdr = kmalloc(sizeof(ip_header_t));
    
    hdr->version_ihl = (IP(4) << 4) | ((uint8_t) (sizeof(ip_header_t) / sizeof(uint32_t)));
    hdr->dscp_ecn = 0;
    hdr->total_length = swap_uint16(sizeof(ip_header_t) + packet->tran_len + packet->payload_len);
    hdr->ident = 0;
    hdr->flags_frag_off = IP_FLAG_DONT_FRAG;
    hdr->ttl = 0x40;
    hdr->protocol = protocol;
    hdr->src = src;
    hdr->dst = dst;
    hdr->checksum = 0;
    
    uint32_t sum = 0;
    for (uint32_t len = 0; len < (sizeof(ip_header_t) / sizeof(uint32_t)); len++) {
        sum += ((uint16_t *) hdr)[len];
    }
        
    hdr->checksum = sum_to_checksum(sum);
 
    packet->net_hdr = hdr;
    packet->net_len = sizeof(ip_header_t);
}

void layer_tran_udp(net_packet_t *packet, ip_t src, ip_t dst, uint16_t src_port, uint16_t dst_port) {    
    udp_header_t *hdr = kmalloc(sizeof(udp_header_t));
    
    hdr->src_port = swap_uint16(src_port);
    hdr->dst_port = swap_uint16(dst_port);
    hdr->length = swap_uint16(sizeof(udp_header_t) + packet->payload_len);
    
    uint32_t sum = 0;
    sum += ((uint16_t *) &src)[0];
    sum += ((uint16_t *) &src)[1];
    sum += ((uint16_t *) &dst)[0];
    sum += ((uint16_t *) &dst)[1];    
    sum += swap_uint16((uint16_t) IP_PROT_UDP);
    sum += swap_uint16(sizeof(udp_header_t) + packet->payload_len);
    
    sum += hdr->src_port;
    sum += hdr->dst_port;
    sum += hdr->length;
    
    uint32_t len = packet->payload_len / sizeof(uint16_t);
    uint16_t *ptr = (uint16_t *) packet->payload;
    for (; len > 1; len--) {
        sum += *ptr++;
    }
    
    if(len) {
        sum += *((uint8_t *) ptr);
    }
    
    hdr->checksum = sum_to_checksum(sum);
    
    packet->tran_hdr = hdr;
    packet->tran_len = sizeof(udp_header_t);
}
