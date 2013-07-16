#ifndef KERNEL_NET_TYPES_H
#define KERNEL_NET_TYPES_H

#include "lib/int.h"
#include "common/list.h"

typedef struct packet packet_t;

typedef struct sock_addr {
    uint32_t family;
    void *addr;
} sock_addr_t;

#define PROTOCOL_IP  0
#define PROTOCOL_ARP 1
typedef uint32_t net_protocol_t;

typedef struct sock_route {
    sock_addr_t src;
    sock_addr_t dst;

    net_protocol_t protocol;
} sock_route_t;

typedef struct net_interface net_interface_t;

typedef enum packet_state {
    P_UNRESOLVED,
    P_RESOLVED
} packet_state_t;

typedef struct net_link_layer {
    void (*resolve)(packet_t *packet);
    void (*build_hdr)(packet_t *);
    void (*recieve)(packet_t *, void *, uint16_t);
} net_link_layer_t;

typedef struct sock_buff {
    uint32_t size;
    void *buff;
} sock_buff_t;

struct packet {
    list_head_t list;

    packet_state_t state;
    net_interface_t *interface;

    sock_route_t route;

    sock_buff_t link;
    sock_buff_t net;
    sock_buff_t tran;
    sock_buff_t payload;
};

#endif
