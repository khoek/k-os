#ifndef KERNEL_NET_PROTOCOLS_H
#define KERNEL_NET_PROTOCOLS_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/types.h"

//Link layer
#define HTYPE_ETH  1

#define ETH_TYPE_IP  0x0800
#define ETH_TYPE_ARP 0x0806

typedef struct ethernet_header {
    mac_t dst;
    mac_t src;
    uint16_t type;
} PACKED ethernet_header_t;

//Network layer
#define ARP_OP_REQUEST  0x0001
#define ARP_OP_RESPONSE 0x0002

typedef struct arp_header {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t op;
    mac_t sender_mac;
    ip_t sender_ip;
    mac_t target_mac;
    ip_t target_ip;
} PACKED arp_header_t;

#define IP_PROT_ICMP 0x01
#define IP_PROT_TCP  0x06
#define IP_PROT_UDP  0x11

#define IP_VERSION(x)   ((x & 0xF0) >> 4)
#define IP_IHL(x)       ((x & 0x0F))
#define IP_DSCP(x)      ((x & 0xFC) >> 2)
#define IP_ECN(x)       ((x & 0x03))
#define IP_FLAGS(x)     ((x & 0x00E0) >> 5)
#define IP_FRAG_OFF(x)  ((x & 0xFF1F))

#define IP_FLAG_EVIL       (1 << 5) //RFC 3514 :D
#define IP_FLAG_DONT_FRAG  (1 << 6)
#define IP_FLAG_MORE_FRAGS (1 << 7)

#define IP(version) (version)

typedef struct ip_header {
    uint8_t  version_ihl;
    uint8_t  dscp_ecn;
    uint16_t total_length;
    uint16_t ident;
    uint16_t flags_frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    ip_t src;
    ip_t dst;
} PACKED ip_header_t;

//Transport layer
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

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} PACKED udp_header_t;

//Packet
struct net_packet {
    union {
        void *ptr;
        ethernet_header_t *eth;
    } link;
    uint32_t link_size;
    
    union {
        void *ptr;
        arp_header_t *arp;
        ip_header_t *ip;
    } net;
    uint32_t net_size;
    
    union {
        void *ptr;
        icmp_header_t *icmp;
        tcp_header_t *tcp;
        udp_header_t *udp;
    } tran;
    uint32_t tran_size;
    
    void *payload;
    uint32_t payload_size;
};

#endif
