#ifndef KERNEL_ETHERNET_H
#define KERNEL_ETHERNET_H

#include "int.h"

void ethernet_handle(uint8_t *packet, uint16_t length);

#endif
