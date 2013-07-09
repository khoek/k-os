#include "lib/int.h"
#include "common/swap.h"
#include "mm/cache.h"
#include "net/types.h"

#include "checksum.h"
#include "ip.h"

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
