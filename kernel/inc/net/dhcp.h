#ifndef KERNEL_DHCP_H
#define KERNEL_DHCP_H

#include "net/interface.h"

void dhcp_start(net_interface_t *interface);

#endif