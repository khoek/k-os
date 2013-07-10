#ifndef KERNEL_NET_NBNS_H
#define KERNEL_NET_NBNS_H

#include "lib/int.h"
#include "net/types.h"

#define NBNS_PORT 137

void nbns_register_name(net_interface_t *interface, char *name);
void nbns_handle(net_interface_t *interface, packet_t *packet, void *raw, uint16_t len);

#endif
