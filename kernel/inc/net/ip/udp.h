#ifndef KERNEL_NET_IP_UDP_H
#define KERNEL_NET_IP_UDP_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/socket.h"

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} PACKED udp_header_t;

extern sock_protocol_t udp_protocol;

#endif
