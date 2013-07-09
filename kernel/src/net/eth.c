#include "lib/int.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "video/log.h"

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
