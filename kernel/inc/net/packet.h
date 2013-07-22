#ifndef KERNEL_NET_PACKET_H
#define KERNEL_NET_PACKET_H

typedef struct packet packet_t;

#include "lib/int.h"
#include "common/list.h"
#include "sync/semaphore.h"
#include "net/socket.h"
#include "net/interface.h"

typedef enum packet_state {
    P_UNRESOLVED,
    P_RESOLVED
} packet_state_t;

struct packet {
    list_head_t list;

    semaphore_t *dispatch_lock;

    packet_state_t state;
    net_interface_t *interface;

    sock_route_t route;

    sock_buff_t link;
    sock_buff_t net;
    sock_buff_t tran;
    sock_buff_t payload;
};

packet_t * packet_create(net_interface_t *interface, void *payload, uint16_t len);
void packet_destroy(packet_t *packet);
void packet_send(packet_t *packet);
uint32_t packet_expand(void *buff, packet_t *packet, uint32_t minimum_size);

#endif
