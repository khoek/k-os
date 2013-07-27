#ifndef KERNEL_NET_IP_ICMP_H
#define KERNEL_NET_IP_ICMP_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/socket.h"

#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_ECHO_REPLY   0

#define ICMP_CODE_ECHO_REQUEST 0
#define ICMP_CODE_ECHO_REPLY   0

typedef struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t other;
} PACKED icmp_header_t;

extern sock_protocol_t icmp_protocol;

void icmp_build(packet_t *packet, uint8_t type, uint8_t code, uint32_t other, ip_t dst_ip);
void icmp_handle(packet_t *packet, void *raw, uint16_t len);

#endif
