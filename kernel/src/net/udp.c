#include "lib/int.h"
#include "common/swap.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "video/log.h"

#include "checksum.h"
#include "ip.h"
#include "udp.h"

void layer_tran_udp(net_packet_t *packet, mac_t src_mac, mac_t dst_mac, ip_t src_ip, ip_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    udp_header_t *hdr = kmalloc(sizeof(udp_header_t));

    hdr->src_port = swap_uint16(src_port);
    hdr->dst_port = swap_uint16(dst_port);
    hdr->length = swap_uint16(sizeof(udp_header_t) + packet->payload_len);

    uint32_t sum = 0;
    sum += ((uint16_t *) &src_ip)[0];
    sum += ((uint16_t *) &src_ip)[1];
    sum += ((uint16_t *) &dst_ip)[0];
    sum += ((uint16_t *) &dst_ip)[1];
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

    layer_net_ip(packet, IP_PROT_UDP, src_mac, dst_mac, src_ip, dst_ip);
}

void recv_tran_udp(net_interface_t *interface, void *packet, uint16_t len) {
    udp_header_t *udp = (udp_header_t *) packet;
    packet += sizeof(udp_header_t);
    len -= sizeof(udp_header_t);

    logf("udp - src: %u dst: %u", swap_uint16(udp->src_port), swap_uint16(udp->dst_port));
}
