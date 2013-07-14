#ifndef KERNEL_NET_IP_TCP_H
#define KERNEL_NET_IP_TCP_H

#include "lib/int.h"
#include "common/compiler.h"

#define TCP_DATA_OFF(x) ((x & 0xF000) >> 4)
#define TCP_FLAGS(x)    ((x & 0x0FFF) >> 4)

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

#endif
