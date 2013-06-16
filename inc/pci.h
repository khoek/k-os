#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H

#include "common.h"

#define BAR_TYPE_32     0x00
#define BAR_TYPE_64     0x02

#define BAR_TYPE(x)     (x & 0x7)
#define BAR_ADDR_32(x)  (x & 0xFFFFFFFC)
#define BAR_ADDR_64(x)  ((x & 0xFFFFFFF0) + ((*((&x) + 1) & 0xFFFFFFFF) << 32))

typedef struct pci_device {
    uint32_t class, subclass, vendor, device, progid, revision, interrupt;
    uint32_t bar[6];
} pci_device_t;

#endif
