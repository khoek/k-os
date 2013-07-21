#ifndef KERNEL_NET_IP_TCP_H
#define KERNEL_NET_IP_TCP_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/socket.h"

#define TCP_DATA_OFF(x) ((x & 0xF000) >> 12)
#define TCP_FLAGS(x)    (x & 0x00FF)

typedef struct tcp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t data_off_flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent;
} PACKED tcp_header_t;

extern sock_protocol_t tcp_protocol;

void tcp_recv(packet_t *packet, void *raw, uint16_t len);

#endif
