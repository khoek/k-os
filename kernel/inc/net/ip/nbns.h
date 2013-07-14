#ifndef KERNEL_NET_IP_NBNS_H
#define KERNEL_NET_IP_NBNS_H

#include "lib/int.h"
#include "common/compiler.h"
#include "net/types.h"

#define NBNS_PORT 137

void nbns_register_name(net_interface_t *interface, char *name);
void nbns_handle(packet_t *packet, void *raw, uint16_t len);

#endif
