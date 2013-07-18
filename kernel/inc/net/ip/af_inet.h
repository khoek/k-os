#ifndef KERNEL_NET_IP_PF_INET_H
#define KERNEL_NET_IP_PF_INET_H

#include "lib/int.h"
#include "common/list.h"
#include "common/compiler.h"
#include "net/socket.h"

#define IPPROTO_RAW 0
#define IPPROTO_TCP 1

typedef struct pf_inet_protocol_t {
    list_head_t list;

    uint32_t type;
    uint16_t protocol;
    sock_protocol_t *impl;
} pf_inet_protocol_t;

void register_pf_inet_protocol(pf_inet_protocol_t *protocol);

#endif
