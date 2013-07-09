#ifndef KERNEL_NET_TYPES_H
#define KERNEL_NET_TYPES_H

#include "lib/int.h"

typedef struct mac { uint8_t addr[6]; } mac_t;
typedef struct ip { uint8_t addr[4]; } ip_t;

#define MAC_BROADCAST ((mac_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} })
#define MAC_NONE ((mac_t) { .addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} })

#define IP_BROADCAST ((ip_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF} })
#define IP_NONE ((ip_t) { .addr = {0x00, 0x00, 0x00, 0x00} })

typedef struct packet packet_t;
typedef struct net_interface net_interface_t;

#endif
