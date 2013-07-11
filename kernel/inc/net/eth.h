#ifndef KERNEL_NET_ETH
#define KERNEL_NET_ETH

#include "lib/int.h"
#include "net/interface.h"

void eth_recieve(packet_t *packet, void *raw, uint16_t len);

#endif
