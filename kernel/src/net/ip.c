#include "lib/int.h"
#include "common/swap.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "video/log.h"

#include "checksum.h"
#include "eth.h"
#include "ip.h"

void layer_net_ip(net_packet_t *packet, uint8_t protocol, mac_t src_mac, mac_t dst_mac, ip_t src, ip_t dst) {
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

    layer_link_eth(packet, ETH_TYPE_IP, src_mac, dst_mac);
}

void recv_net_ip(void *packet, uint16_t len) {
    ip_header_t *ip = (ip_header_t *) packet;
    packet += sizeof(ip_header_t);
    len -= sizeof(ip_header_t);

    if(IP_VERSION(ip->version_ihl) != 0x04) {
        logf("ip - unsupported version number (0x%02X)", IP_VERSION(ip->version_ihl));
    } else {
        logf("ip - src %u.%u.%u.%u dst %u.%u.%u.%u",
            ip->src.addr[0], ip->src.addr[1], ip->src.addr[2], ip->src.addr[3],
            ip->dst.addr[0], ip->dst.addr[1], ip->dst.addr[2], ip->dst.addr[3]
        );

        switch(ip->protocol) {
            case IP_PROT_TCP: {
                recv_tran_tcp(packet, len);
                break;
            }
            case IP_PROT_UDP: {
                recv_tran_udp(packet, len);
                break;
            }
            default: {
                logf("ip - unrecognised protocol (0x%02X)", ip->protocol);
                break;
            }
        }
    }
}
