#include "init.h"
#include "pci.h"
#include "panic.h"
#include "asm.h"
#include "ide.h"
#include "ahci.h"
#include "net.h"
#include "log.h"

#define CONFIG_ADDRESS  0xCF8
#define CONFIG_DATA     0xCFC

#define REG_FULL_BAR0   0x10
#define REG_FULL_BAR1   0x14
#define REG_FULL_BAR2   0x18
#define REG_FULL_BAR3   0x1C
#define REG_FULL_BAR4   0x20
#define REG_FULL_BAR5   0x24

#define REG_WORD_VENDOR 0x00
#define REG_WORD_DEVICE 0x02

//Header type 0x00
#define REG_BYTE_REVISN 0x08
#define REG_BYTE_PROGID 0x09
#define REG_BYTE_SCLASS 0x0A
#define REG_BYTE_CLASS  0x0B
#define REG_BYTE_HEADER 0x0E
#define REG_BYTE_INTRPT 0x3C

//Header type 0x01
#define REG_BYTE_2NDBUS 0x19

static uint32_t pci_read(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) {
    outl(CONFIG_ADDRESS, (uint32_t)((((uint32_t) 0x80000000) | (uint32_t) bus << 16) | ((uint32_t) slot << 11) | ((uint32_t) func << 8) | (offset & 0xFC)));
    return inl(CONFIG_DATA);
}

static uint16_t pci_read_word(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) {
    outl(CONFIG_ADDRESS, (uint32_t)((((uint32_t) 0x80000000) | (uint32_t) bus << 16) | ((uint32_t) slot << 11) | ((uint32_t) func << 8) | (offset & 0xFC)));
    return (uint16_t) ((inl(CONFIG_DATA) >> ((offset) * 8)) & 0xFFFF);
}

static uint8_t pci_read_byte(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) {
    return (uint8_t) (pci_read_word(bus, slot, func, offset) & 0xFF);
}

static char* classes[] = {
    [0x00] =   "Unspecified Type",
    [0x01] =   "Mass Storage Controller",
    [0x02] =   "Network Controller",
    [0x03] =   "Display Controller",
    [0x04] =   "Multimedia Controller",
    [0x05] =   "Memory Controller",
    [0x06] =   "Bridge Device",
    [0x07] =   "Simple Communication Controller",
    [0x08] =   "Base System Peripheral",
    [0x09] =   "Input Device",
    [0x0A] =  "Docking Station",
    [0x0B] =  "Processor",
    [0x0C] =  "Serial Bus Controller",
    [0x0D] =  "Wireless Controller",
    [0x0E] =  "Intelligent I/O Controller",
    [0x0F] =  "Satellite Communication Controller",
    [0x10] =  "Encryption/Decryption Controller",
    [0x11] =  "Data Acquisition and Signal Processing Controller",
    [0xFF] = "Miscellaneous Device"
};

static void probe_bus(uint8_t bus);
static void probe_device(uint8_t bus, uint8_t device);
static void probe_function(uint8_t bus, uint8_t device, uint8_t function);

static void probe_bus(uint8_t bus) {
    for(uint8_t device = 0; device < 32; device++) {
        probe_device(bus, device);
    }
}

static void probe_device(uint8_t bus, uint8_t device) {
    uint16_t vendor = pci_read_word(bus, device, 0, REG_WORD_VENDOR);

    if(vendor == 0x0000 || vendor == 0xFFFF) return;

    probe_function(bus, device, 0);
    if((pci_read_byte(bus, device, 0, REG_BYTE_HEADER) & 0x80) != 0) {
        for(int function = 1; function < 8; function++) {
             if((pci_read_word(bus, device, function, REG_WORD_VENDOR)) != 0xFFFF) {
                 probe_function(bus, device, function);
             }
        }
    }
}

static void probe_function(uint8_t bus, uint8_t device, uint8_t function) {
    pci_device_t dev;

    dev.class = pci_read_byte(bus, device, function, REG_BYTE_CLASS);
    dev.subclass = pci_read_byte(bus, device, function, REG_BYTE_SCLASS);
    dev.vendor = pci_read_word(bus, device, function, REG_WORD_VENDOR);
    dev.device = pci_read_word(bus, device, function, REG_WORD_DEVICE);
    dev.progid = pci_read_byte(bus, device, function, REG_BYTE_PROGID);
    dev.revision = pci_read_byte(bus, device, function, REG_BYTE_REVISN);
    dev.bar[0] = pci_read(bus, device, function, REG_FULL_BAR0);
    dev.bar[1] = pci_read(bus, device, function, REG_FULL_BAR1);
    dev.bar[2] = pci_read(bus, device, function, REG_FULL_BAR2);
    dev.bar[3] = pci_read(bus, device, function, REG_FULL_BAR3);
    dev.bar[4] = pci_read(bus, device, function, REG_FULL_BAR4);
    dev.bar[5] = pci_read(bus, device, function, REG_FULL_BAR5);
    dev.interrupt = pci_read_byte(bus, device, function, REG_BYTE_INTRPT);

    logf("pci - %02X:%02X:%02X %02X:%02X:%02X:%02X %04X:%04X - %s",
        bus, device, function, dev.class, dev.subclass, dev.progid, dev.revision, dev.vendor, dev.device,
        (classes[dev.class] ? classes[dev.class] : "Unknown Type"));

    switch(dev.class) {
        case 0x01:
             switch(dev.subclass) {
                 case 0x01:
                     ide_init(dev);
                     break;
                 case 0x06:
                     ahci_init(dev);
                     break;
             }
             break;
        case 0x02:
             switch(dev.subclass) {
                 case 0x00:
                     net_825xx_init(dev);
                     break;
             }
             break;
        case 0x06:
             switch(dev.subclass) {
                 case 0x04:
                     probe_bus(pci_read_byte(bus, device, function, REG_BYTE_2NDBUS));
                     break;
             }
             break;
    }
}

static INITCALL pci_init() {
    outl(CONFIG_ADDRESS, 0x80000000);
    if(inl(CONFIG_ADDRESS) != 0x80000000) {
        return 1;
    }

    if((pci_read_byte(0, 0, 0, REG_BYTE_HEADER) & 0x80) == 0) {
        //Single PCI host controller
        probe_bus(0);
    } else {
        //Multiple PCI host controllers
        for(unsigned int function = 0; function < 8; function++) {
             if(pci_read_word(0, 0, function, REG_WORD_VENDOR) != 0xFFFF) break;
             probe_bus(function);
        }
    }

    return 0;
}

subsys_initcall(pci_init);
