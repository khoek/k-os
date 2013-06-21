#include "common.h"
#include "ethernet.h"
#include "log.h"

#define ETHERNET_TYPE_IP    0x0008
#define ETHERNET_TYPE_ARP   0x0608

typedef struct ethernet_header {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;
} PACKED ethernet_header_t;

#define IP_VERSION(x)   ((x & 0xF0) >> 4)
#define IP_IHL(x)       ((x & 0x0F))
#define IP_DSCP(x)      ((x & 0xFC) >> 2)
#define IP_ECN(x)       ((x & 0x03))
#define IP_FLAGS(x)     ((x & 0x00E0) >> 5)
#define IP_FRAG_OFF(x)  ((x & 0xFF1F))

#define IP_TYPE_TCP 0x06
#define IP_TYPE_UDP 0x11

typedef struct ip_header {
    uint8_t  version_ihl;
    uint8_t  dscp_ecn;
    uint16_t total_length;
    uint16_t ident;
    uint16_t flags_frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t src[4];
    uint8_t dst[4];
} PACKED ip_header_t;

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} PACKED udp_header_t;

void ethernet_handle(uint8_t *packet, uint16_t length) {
    ethernet_header_t *header = (ethernet_header_t *) packet;
    packet += sizeof(ethernet_header_t);
    length -= sizeof(ethernet_header_t);

    switch(header->type) {
        case ETHERNET_TYPE_IP: {
            ip_header_t *ip = (ip_header_t *) packet;
            packet += sizeof(ip_header_t);
            length -= sizeof(ip_header_t);

            if(IP_VERSION(ip->version_ihl) != 0x04) {
                logf("ethernet - ip packet with unsupported version number (0x%02X)", IP_VERSION(ip->version_ihl));
            } else {
                switch(ip->protocol) {
                    case IP_TYPE_TCP: {
                        tcp_header_t *tcp = (tcp_header_t *) packet;
                        packet += sizeof(tcp_header_t);
                        length -= sizeof(tcp_header_t);

                        logf("ethernet - tcp packet detected");
                        logf("ethernet - src %u.%u.%u.%u port %u dst %u.%u.%u.%u port %u",
                            ip->src[0], ip->src[1], ip->src[2], ip->src[3], tcp->src_port,
                            ip->dst[0], ip->dst[1], ip->dst[2], ip->dst[3], tcp->dst_port
                        );

                        logf("payload: %u", length);

                        break;
                    }
                    case IP_TYPE_UDP: {
                        udp_header_t *udp = (udp_header_t *) packet;
                        packet += sizeof(udp_header_t);
                        length -= sizeof(udp_header_t);
                        
                        logf("ethernet - udp packet detected");
                        logf("ethernet - src %u.%u.%u.%u port %u dst %u.%u.%u.%u port %u",
                            ip->src[0], ip->src[1], ip->src[2], ip->src[3], udp->src_port,
                            ip->dst[0], ip->dst[1], ip->dst[2], ip->dst[3], udp->dst_port
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
        case ETHERNET_TYPE_ARP: {
            logf("ethernet - arp packet detected");
            break;
        }
        default: {
            logf("ethernet - unrecognised packet type (0x%02X)", header->type);
            break;
        }
    }
}
