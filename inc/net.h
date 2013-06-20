#ifndef KERNEL_NET_H
#define KERNEL_NET_H

#include "pci.h"

int32_t net_send(void *packet, uint16_t length);
void net_825xx_init(pci_device_t dev);

#endif
