#ifndef KERNEL_AHCI_H
#define KERNEL_AHCI_H

#include "init.h"
#include "pci.h"

void __init ahci_init(pci_device_t dev);

#endif
