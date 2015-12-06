#ifndef KERNEL_DRIVER_BUS_PCI_H
#define KERNEL_DRIVER_BUS_PCI_H

#include "device/device.h"

#define PCI_ID_ANY      ((uint16_t) ~0)

#define BAR_TYPE_32     0x00
#define BAR_TYPE_64     0x02

#define PCI_FULL_BAR0   0x10
#define PCI_FULL_BAR1   0x14
#define PCI_FULL_BAR2   0x18
#define PCI_FULL_BAR3   0x1C
#define PCI_FULL_BAR4   0x20
#define PCI_FULL_BAR5   0x24
#define PCI_FULL_CMDSTA 0x04
#define PCI_FULL_CLASS  0x08

#define PCI_WORD_VENDOR 0x00
#define PCI_WORD_DEVICE 0x02

//Header type 0x00
#define PCI_BYTE_REVISN 0x08
#define PCI_BYTE_PROGIF 0x09
#define PCI_BYTE_SCLASS 0x0A
#define PCI_BYTE_CLASS  0x0B
#define PCI_BYTE_HEADER 0x0E
#define PCI_BYTE_INTRPT 0x3C

//Header type 0x01
#define PCI_BYTE_2NDBUS 0x19

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
    volatile uint8_t interrupt;
    uint32_t bar[6];
} pci_device_t;

typedef struct pci_driver {
    driver_t driver;
    pci_ident_t *supported;
    uint32_t supported_len;
} pci_driver_t;

extern bus_t pci_bus;

uint32_t pci_readl(pci_loc_t loc, uint16_t offset);
uint16_t pci_readw(pci_loc_t loc, uint16_t offset);
uint8_t pci_readb(pci_loc_t loc, uint16_t offset);

void pci_writel(pci_loc_t loc, uint16_t offset, uint32_t val);

#endif
