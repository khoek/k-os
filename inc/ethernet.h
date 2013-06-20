#ifndef KERNEL_ETHERNET_H
#define KERNEL_ETHERNET_H

#include <stdint.h>

void ethernet_handle(uint8_t *packet, uint16_t length);

#endif
