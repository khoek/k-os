#include "lib/int.h"
#include "common/init.h"
#include "sync/spinlock.h"
#include "net/socket.h"
#include "net/types.h"
#include "net/ip/pf_inet.h"
#include "net/ip/ip.h"
#include "net/ip/tcp.h"
#include "net/ip/udp.h"
#include "net/ip/icmp.h"
#include "net/ip/raw.h"

static list_head_t types[SOCK_MAX];
static DEFINE_SPINLOCK(type_lock);

void register_pf_inet_protocol(pf_inet_protocol_t *protocol) {
    uint32_t flags;
    spin_lock_irqsave(&type_lock, &flags);

    list_add_before(&protocol->list, &types[protocol->type]);

    spin_unlock_irqstore(&type_lock, flags);
}

static sock_protocol_t * pf_inet_find(uint32_t type, uint32_t protocol) {
    bool success;
    pf_inet_protocol_t *found;
    LIST_FOR_EACH_ENTRY(found, &types[protocol], list) {
        success = true;
        if(found->protocol == protocol || found->protocol == IPPROTO_RAW) {
            break;
        } else if(protocol == IPPROTO_RAW) {
            protocol = found->protocol;
            break;
        }
        success = false;
    }

    if(!success) return NULL;

    return found->impl;
}

static sock_family_t pf_inet = {
    .code = PF_INET,
    .find = pf_inet_find
};

static pf_inet_protocol_t builtin_protocols[] = {
    {
        .type     = SOCK_DGRAM,
        .protocol = IP_PROT_UDP,
        .impl     = &udp_protocol
    },
    {
        .type     = SOCK_DGRAM,
        .protocol = IP_PROT_ICMP,
        .impl     = &icmp_protocol
    },
    {
        .type     = SOCK_RAW,
        .protocol = 0,
        .impl     = &raw_protocol
    },
};

static INITCALL pf_inet_init() {
    for(uint32_t i = 0; i < SOCK_MAX; i++) {
        list_init(&types[i]);
    }

    for(uint32_t i = 0; i < sizeof(builtin_protocols) / sizeof(pf_inet_protocol_t); i++) {
        register_pf_inet_protocol(&builtin_protocols[i]);
    }

    register_sock_family(&pf_inet);

    return 0;
}

core_initcall(pf_inet_init);
