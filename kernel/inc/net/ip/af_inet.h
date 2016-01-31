#ifndef KERNEL_NET_IP_PF_INET_H
#define KERNEL_NET_IP_PF_INET_H

#include "common/types.h"
#include "common/list.h"
#include "common/compiler.h"
#include "net/socket.h"

#define IPPROTO_RAW 0
#define IPPROTO_TCP 1

typedef struct inet_protocol_t {
    list_head_t list;

    uint16_t protocol;
    sock_protocol_t *impl;
} inet_protocol_t;

extern sock_family_t af_inet;

void register_inet_protocol(inet_protocol_t *protocol);

#endif
