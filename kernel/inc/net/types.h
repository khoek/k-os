#ifndef KERNEL_NET_TYPES_H
#define KERNEL_NET_TYPES_H

#include "lib/int.h"

typedef struct mac { uint8_t addr[6]; } mac_t;
typedef struct ip { uint8_t addr[4]; } ip_t;

#define MAC_BROADCAST ((mac_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} })
#define MAC_NONE ((mac_t) { .addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} })

#define IP_BROADCAST ((ip_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF} })
#define IP_NONE ((ip_t) { .addr = {0x00, 0x00, 0x00, 0x00} })

typedef union hard_addr {
    mac_t mac;
} hard_addr_t;

typedef union sock_addr {
    ip_t ip;
} sock_addr_t;

#define PROTOCOL_IP  0
#define PROTOCOL_ARP 1
typedef uint32_t net_protocol_t;

typedef struct hard_route {
    hard_addr_t *src;
    hard_addr_t *dst;
} hard_route_t;

typedef struct sock_route {
    sock_addr_t *src;
    sock_addr_t *dst;
} sock_route_t;

typedef struct route {
    union {
        hard_route_t hard;
        sock_route_t sock;
    };

    net_protocol_t protocol;
} route_t;

typedef struct net_interface net_interface_t;

typedef enum packet_state {
    P_UNRESOLVED,
    P_RESOLVED
} packet_state_t;

#include "net/protocols.h"

typedef struct packet {
    packet_state_t state;

    net_interface_t *interface;
    route_t route;

    union {
        void *ptr;
        eth_header_t *eth;
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
} packet_t;

typedef struct net_link_layer {
    void (*resolve)(packet_t *packet);
    void (*build_hdr)(packet_t *);
    void (*recieve)(packet_t *, void *, uint16_t);
} net_link_layer_t;

#endif
