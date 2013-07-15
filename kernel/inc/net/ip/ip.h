#ifndef KERNEL_NET_IP_IP_H
#define KERNEL_NET_IP_IP_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/types.h"
#include "net/socket.h"

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

#endif
