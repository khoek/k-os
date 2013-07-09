#include "lib/int.h"
#include "lib/string.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "video/log.h"

#include "checksum.h"

void layer_tran_icmp(packet_t *packet, uint8_t type, uint8_t code, uint32_t other, mac_t src_mac, mac_t dst_mac, ip_t src_ip, ip_t dst_ip) {
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

    layer_net_ip(packet, IP_PROT_ICMP, src_mac, dst_mac, src_ip, dst_ip);    
}

void recv_tran_icmp(net_interface_t *interface, void *packet, uint16_t len) {
    icmp_header_t *icmp = packet;
    packet = icmp + 1;
    len -= sizeof(icmp_header_t);
    
    logf("icmp - type: %u code: %u", icmp->type, icmp->code);
    
    /*switch(icmp->type) {
        case ICMP_TYPE_ECHO_REQUEST: {
            switch(icmp->type) {
                case ICMP_CODE_ECHO_REPLY: {
                    void *buff = kmalloc(len);
                    memcpy(buff, packet, len);
                    
                    packet_t *reply = packet_alloc(buff, len);            
                    layer_tran_icmp(reply, ICMP_TYPE_ECHO_REPLY, ICMP_CODE_ECHO_REPLY, interface->mac, ->sender_mac, interface->ip, arp->sender_ip);
                    packet_send(interface, reply);
                    packet_free(reply);
                    
                    kfree(buff, len);
                    
                    break;
                }
            }
            break;
        }
    }*/
}
