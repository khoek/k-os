#include "common.h"
#include "ethernet.h"
#include "log.h"

#define TYPE_IP     0x0008
#define TYPE_ARP    0x0608

typedef struct ethernet_header {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} PACKED ethernet_header_t;

#define IP_VERSION(x)   ((x & 0xF0) >> 4)
#define IP_IHL(x)       ((x & 0x0F))
#define IP_DSCP(x)      ((x & 0xFC) >> 2)
#define IP_ECN(x)       ((x & 0x03))
#define IP_FLAGS(x)     ((x & 0x00E0) >> 5)
#define IP_FRAG_OFF(x)  ((x & 0xFF1F))

typedef struct ip_header {
    uint8_t version_ihl;
    uint8_t dscp_ecn;
    uint16_t total_length;
    uint16_t ident;
    uint16_t flags_frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
} PACKED ip_header_t;

void ethernet_handle(uint8_t *packet, uint16_t length) {
    ethernet_header_t *header = (ethernet_header_t *) packet;
    packet += sizeof(ethernet_header_t);

    logf("ethernet - packet type:0x%02X recieved (payload %u/%u bytes)", header->type, length - sizeof(ethernet_header_t), length);

    logf("ethernet - dst %02X:%02X:%02X:%02X:%02X:%02X, src %02X:%02X:%02X:%02X:%02X:%02X",
        header->dst[0], header->dst[1], header->dst[2], header->dst[3], header->dst[4], header->dst[5],
        header->src[0], header->src[1], header->src[2], header->src[3], header->src[4], header->src[5]
    );

    switch(header->type) {
        case TYPE_IP: {
            ip_header_t *ip = (ip_header_t *) packet;
            packet += sizeof(ip_header_t);
            logf("ethernet - ip packet detected (IPv%u)", IP_VERSION(ip->version_ihl));
            logf("ethernet - ihl:%u dscp:%X ecn:%X total_len:%X ident:%X flags:%X frag_off:%X ttl:%X protocol:%X checksum:%X src:%X dst:%X", IP_IHL(ip->version_ihl) * 4, IP_DSCP(ip->dscp_ecn), IP_ECN(ip->dscp_ecn), ip->total_length, ip->ident, IP_FLAGS(ip->flags_frag_off), IP_FRAG_OFF(ip->flags_frag_off), ip->ttl, ip->protocol, ip->checksum, ip->src, ip->dst);
            break;
        }
        case TYPE_ARP: {
            logf("ethernet - arp packet detected");
            break;
        }
        default: {
            logf("ethernet - unrecognised packet type (type:%02X)", header->type);
            break;
        }
    }
}
