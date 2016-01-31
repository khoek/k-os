#ifndef KERNEL_NET_IP_DNS_H
#define KERNEL_NET_IP_DNS_H

#include "common/types.h"
#include "common/compiler.h"

typedef struct dns_header {
    uint16_t xid;
    uint16_t flags;
    uint16_t query_count;
    uint16_t answer_count;
    uint16_t authority_count;
    uint16_t additional_count;
} PACKED dns_header_t;

typedef struct dns_sym_query {
    uint8_t len;
    uint8_t name[32];
    uint8_t zero;
    uint16_t type;
    uint16_t class;
} PACKED dns_sym_query_t;

typedef struct dns_sym_rr {
    uint8_t len;
    uint8_t name[32];
    uint8_t zero;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t addr_len;
    uint16_t nb_flags;
    ip_t ip;
} PACKED dns_sym_rr_t;

typedef struct dns_sym_aliased_rr {
    uint16_t alias;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t addr_len;
    uint16_t nb_flags;
    ip_t ip;
} PACKED dns_sym_aliased_rr_t;

#endif
