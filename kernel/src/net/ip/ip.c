#include "lib/int.h"
#include "lib/string.h"
#include "common/swap.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "net/interface.h"
#include "video/log.h"

#include "checksum.h"

void ip_build(packet_t *packet, uint8_t protocol, ip_t dst) {
    ip_header_t *hdr = kmalloc(sizeof(ip_header_t));

    hdr->version_ihl = (IP(4) << 4) | ((uint8_t) (sizeof(ip_header_t) / sizeof(uint32_t)));
    hdr->dscp_ecn = 0;
    hdr->total_length = swap_uint16(sizeof(ip_header_t) + packet->tran_size + packet->payload_size);
    hdr->ident = 0;
    hdr->flags_frag_off = IP_FLAG_DONT_FRAG;
    hdr->ttl = 0x40;
    hdr->protocol = protocol;
    hdr->src = packet->interface->ip;
    hdr->dst = dst;
    hdr->checksum = 0;

    uint32_t sum = 0;
    for (uint32_t len = 0; len < (sizeof(ip_header_t) / sizeof(uint16_t)); len++) {
        sum += ((uint16_t *) hdr)[len];
    }

    hdr->checksum = sum_to_checksum(sum);

    ip_t *dst_copy = kmalloc(sizeof(ip_t));
    memcpy(dst_copy, &dst, sizeof(ip_t));

    packet->state = P_UNRESOLVED;
    packet->route.sock.src = (sock_addr_t *) &packet->interface->ip;
    packet->route.sock.dst = (sock_addr_t *) dst_copy;

    packet->net.ip = hdr;
    packet->net_size = sizeof(ip_header_t);
}

void ip_recv(packet_t *packet, void *raw, uint16_t len) {
    ip_header_t *ip = packet->net.ip = raw;
    raw += sizeof(ip_header_t);
    len -= sizeof(ip_header_t);

    if(IP_VERSION(ip->version_ihl) != 0x04) {
        logf("ip - unsupported version number (0x%02X)", IP_VERSION(ip->version_ihl));
    } else {
        switch(ip->protocol) {
            case IP_PROT_ICMP: {
                icmp_recv(packet, raw, len);
                break;
            }
            case IP_PROT_TCP: {
                tcp_recv(packet, raw, len);
                break;
            }
            case IP_PROT_UDP: {
                udp_recv(packet, raw, len);
                break;
            }
            default: {
                logf("ip - unrecognised protocol (0x%02X)", ip->protocol);
                break;
            }
        }
    }
}
