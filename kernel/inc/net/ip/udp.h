#ifndef KERNEL_NET_IP_UDP_H
#define KERNEL_NET_IP_UDP_H

#include "common/types.h"
#include "common/compiler.h"
#include "net/socket.h"

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} PACKED udp_header_t;

extern sock_protocol_t udp_protocol;

void udp_build(packet_t *packet, ip_t dst_ip, uint16_t src_port_net, uint16_t dst_port_net);
void udp_handle(packet_t *packet, void *raw, uint16_t len);

#endif
