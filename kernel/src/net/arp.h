#include "lib/int.h"
#include "common/compiler.h"
#include "net/types.h"

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
