#ifndef KERNEL_DRIVER_BUS_PCI_H
#define KERNEL_DRIVER_BUS_PCI_H

#include "device/device.h"

#define PCI_ID_ANY      ((uint16_t) ~0)

#define BAR_TYPE_32     0x00
#define BAR_TYPE_64     0x02

#define BAR_TYPE(x)     (x & 0x7)
#define BAR_ADDR_32(x)  (x & 0xFFFFFFFC)
#define BAR_ADDR_64(x)  ((x & 0xFFFFFFF0) + ((*((&x) + 1) & 0xFFFFFFFF) << 32))

typedef struct pci_ident {
    uint32_t class, class_mask; //class << 24 | subclass << 16 | progif << 8 | revision
    uint16_t vendor, device;
} pci_ident_t;

typedef struct pci_loc {
    uint8_t bus, device, function;
} pci_loc_t;

typedef struct pci_device {
    device_t device;
    pci_ident_t ident;
    pci_loc_t loc;
    uint8_t interrupt;
    uint32_t bar[6];

    void *private;
} pci_device_t;

typedef struct pci_driver {
    driver_t driver;
    pci_ident_t *supported;
    uint32_t supported_len;
} pci_driver_t;

extern bus_t pci_bus;

#endif
