#include "lib/int.h"
#include "lib/rand.h"
#include "lib/string.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "net/interface.h"
#include "video/log.h"

#include "checksum.h"

void icmp_build(packet_t *packet, uint8_t type, uint8_t code, uint32_t other, ip_t dst_ip) {
    icmp_header_t *hdr = kmalloc(sizeof(icmp_header_t));

    hdr->type = type;
    hdr->code = code;
    hdr->other = other;

    uint32_t sum = 0;
    sum += hdr->type;
    sum += hdr->code;
    sum += hdr->other;

    uint32_t len = packet->payload_size / sizeof(uint8_t);
    uint16_t *ptr = (uint16_t *) packet->payload;
    for (; len > 1; len -= 2) {
        sum += *ptr++;
    }

    if(len) {
        sum += *((uint8_t *) ptr);
    }

    hdr->checksum = sum_to_checksum(sum);

    packet->tran.icmp = hdr;
    packet->tran_size = sizeof(icmp_header_t);

    ip_build(packet, IP_PROT_ICMP, dst_ip);
}

void icmp_recv(packet_t *packet, void *raw, uint16_t len) {
    icmp_header_t *icmp = packet->tran.icmp = raw;
    raw = icmp + 1;
    len -= sizeof(icmp_header_t);

    switch(icmp->type) {
        case ICMP_TYPE_ECHO_REQUEST: {
            switch(icmp->code) {
                case ICMP_CODE_ECHO_REQUEST: {
                    void *buff = kmalloc(len);
                    memcpy(buff, raw, len);

                    packet_t *reply = packet_create(packet->interface, buff, len);
                    icmp_build(reply, ICMP_TYPE_ECHO_REPLY, ICMP_CODE_ECHO_REPLY, icmp->other, packet->net.ip->src);
                    packet_send(reply);

                    break;
                }
            }
            break;
        }
    }
}
