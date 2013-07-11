#include "lib/string.h"
#include "lib/rand.h"
#include "common/swap.h"
#include "common/compiler.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "net/interface.h"
#include "net/dns.h"
#include "net/nbns.h"
#include "video/log.h"

#define HOSTNAME_TTL 300000

typedef struct nbns_query_response {
    dns_header_t hdr;
    dns_sym_rr_t rr;
} PACKED nbns_query_response_t;

typedef struct nbns_reg_request {
    dns_header_t hdr;
    dns_sym_query_t query;
    dns_sym_aliased_rr_t rr;
} PACKED nbns_reg_request_t;

#define NBNS_NAME_LENGTH 15

#define FLAGS_TO_R(x) (((uint16_t)(x)) >> 15)
#define FLAGS_TO_OPCODE(x) ((((uint16_t)(x)) & 0x7800) >> 11)
#define FLAGS_TO_NM_FLAGS(x) ((((uint16_t)(x)) & 0x07F0) >> 4)
#define FLAGS_TO_RCODE(x) ((((uint16_t)(x)) & 0x000F))

#define TO_FLAGS(r, opcode, nm_flags, rcode) (((((uint16_t) (r))) << 15) | (((uint16_t) (opcode)) << 11) | (((uint16_t) (nm_flags)) << 4) | ((uint16_t) (rcode)))

#define R_REQUEST  0
#define R_RESPONSE 1

#define OPCODE_QUERY        0
#define OPCODE_REGISTRATION 5
#define OPCODE_RELEASE      6
#define OPCODE_WACK         7
#define OPCODE_REFRESH      8

#define NM_FLAG_BROADCAST           (1 << 0)
#define NM_FLAG_RECURSION_AVALIABLE (1 << 3)
#define NM_FLAG_RECURSION_DESIRED   (1 << 4)
#define NM_FLAG_TRUNCATED           (1 << 5)
#define NM_FLAG_AUTHORITIVE         (1 << 6)

#define RCODE_NONE              0x0
#define RCODE_FORMAT_ERROR      0x1
#define RCODE_SERVER_ERROR      0x2
#define RCODE_UNSUPPORTED_ERROR 0x4
#define RCODE_REFUSED_ERROR     0x5
#define RCODE_ACTIVE_ERROR      0x6
#define RCODE_CONFLICT_ERROR    0x7

#define SCOPE_UNIQUE (0 << 15)
#define SCOPE_GROUP  (1 << 15)

#define NODE_B (0 << 13)
#define NODE_P (1 << 13)
#define NODE_M (2 << 13)

#define RR_TYPE_NAME 0x0020

#define RR_CLASS_INTERNET 0x0001

#define NAME_IS_ALIAS 0xC000

#define NAME_WORKSTATION 0x00
static void nbns_encode_name(uint8_t *buff, char *name, uint8_t type) {
    bool foundEnd = false;
    for(uint8_t i = 0; i < NBNS_NAME_LENGTH; i++) {
        if(!name[i]) foundEnd = true;

        if(foundEnd) {
            buff[i * 2] = (' ' >> 4) + 'A';
            buff[(i * 2) + 1] = (' ' & 0xF) + 'A';
        } else {
            buff[i * 2] = (name[i] >> 4) + 'A';
            buff[(i * 2) + 1] = (name[i] & 0xF) + 'A';
        }
    }

    buff[NBNS_NAME_LENGTH * 2] = (type >> 4) + 'A';
    buff[(NBNS_NAME_LENGTH * 2) + 1] = (type & 0xF) + 'A';
}

void nbns_handle(packet_t *packet, void *raw, uint16_t len) {
    if(len < sizeof(dns_header_t)) return;

    dns_header_t *dns = raw;
    raw = dns + 1;
    len -= sizeof(dns_header_t);

    if(FLAGS_TO_R(swap_uint16(dns->flags)) != R_REQUEST) return;
    if(FLAGS_TO_OPCODE(swap_uint16(dns->flags)) != OPCODE_QUERY) return;
    if(swap_uint16(dns->query_count) != 1) return;

    dns_sym_query_t *query = raw;

    uint8_t encode[32];
    nbns_encode_name(encode, net_get_hostname(), NAME_WORKSTATION);
    net_put_hostname();

    if(memcmp(encode, query->name, 32)) return;

    nbns_query_response_t *response = kmalloc(sizeof(nbns_query_response_t));
    memset(response, 0, sizeof(nbns_query_response_t));

    response->hdr.xid = dns->xid;
    response->hdr.flags = swap_uint16(TO_FLAGS(R_RESPONSE, OPCODE_QUERY, NM_FLAG_RECURSION_DESIRED | NM_FLAG_AUTHORITIVE, RCODE_NONE));
    response->hdr.answer_count = swap_uint16(1);

    response->rr.len = sizeof(response->rr.name);
    memcpy(&response->rr.name, encode, response->rr.len);

    response->rr.type = swap_uint16(RR_TYPE_NAME);
    response->rr.class = swap_uint16(RR_CLASS_INTERNET);
    response->rr.ttl = swap_uint32(HOSTNAME_TTL);
    response->rr.addr_len = swap_uint16(sizeof(ip_t) + sizeof(uint16_t));
    response->rr.ip = packet->interface->ip;

    packet_t *resp = packet_create(packet->interface, response, sizeof(nbns_query_response_t));
    udp_build(resp, packet->net.ip->src, NBNS_PORT, NBNS_PORT);
    packet_send(resp);
}

void nbns_register_name(net_interface_t *interface, char *name) {
    nbns_reg_request_t *nbns = kmalloc(sizeof(nbns_reg_request_t));
    memset(nbns, 0, sizeof(nbns_reg_request_t));

    nbns->hdr.xid = rand16();
    nbns->hdr.flags = swap_uint16(TO_FLAGS(R_REQUEST, OPCODE_REGISTRATION, NM_FLAG_RECURSION_DESIRED | NM_FLAG_BROADCAST, RCODE_NONE));
    nbns->hdr.query_count = swap_uint16(1);
    nbns->hdr.additional_count = swap_uint16(1);

    nbns_encode_name(nbns->query.name, name, NAME_WORKSTATION);
    nbns->query.len = sizeof(nbns->query.name);
    nbns->rr.alias = swap_uint16(NAME_IS_ALIAS | offsetof(nbns_reg_request_t, query));

    nbns->query.type = nbns->rr.type = swap_uint16(RR_TYPE_NAME);
    nbns->query.class = nbns->rr.class = swap_uint16(RR_CLASS_INTERNET);

    nbns->rr.ttl = 0;
    nbns->rr.addr_len = swap_uint16(sizeof(ip_t) + sizeof(uint16_t));
    nbns->rr.nb_flags = swap_uint16(SCOPE_UNIQUE | NODE_B);
    nbns->rr.ip = interface->ip;

    packet_t *packet = packet_create(interface, nbns, sizeof(nbns_reg_request_t));
    udp_build(packet, IP_BROADCAST, NBNS_PORT, NBNS_PORT);
    packet_send(packet);
}
