#ifndef KERNEL_NET_IP_TCP_H
#define KERNEL_NET_IP_TCP_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/socket.h"

#define TCP_DATA_OFF(x) ((x & 0x00F0) >> 4)

#define TCP_FLAG_NS  (1 << 0)  //ECN-nonce concealment protection
#define TCP_FLAG_CWR (1 << 15) //Congestion Window Reduced
#define TCP_FLAG_ECE (1 << 14) //ECN-Echo indication
#define TCP_FLAG_URG (1 << 13) //Urgent pointer field is valid
#define TCP_FLAG_ACK (1 << 12) //Acknowledgment field is valid
#define TCP_FLAG_PSH (1 << 11) //Push function
#define TCP_FLAG_RST (1 << 10) //Reset the connection
#define TCP_FLAG_SYN (1 << 9)  //Synchronize sequence numbers
#define TCP_FLAG_FIN (1 << 8)  //No more data from sender

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

void tcp_handle(packet_t *packet, void *raw, uint16_t len);

#endif
