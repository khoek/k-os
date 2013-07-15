#ifndef KERNEL_NET_PACKET_H
#define KERNEL_NET_PACKET_H

#include "lib/int.h"
#include "net/types.h"

packet_t * packet_create(net_interface_t *interface, void *payload, uint16_t len);
void packet_send(packet_t *packet);
uint32_t packet_expand(void *buff, packet_t *packet, uint32_t minimum_size);

#endif
