#include "pci.h"
#include "panic.h"
#include "io.h"
#include "ide.h"
#include "console.h"

#define CONFIG_ADDRESS	0xCF8
#define CONFIG_DATA	0xCFC

#define REG_FULL_BAR0	0x10
#define REG_FULL_BAR1	0x14
#define REG_FULL_BAR2	0x18
#define REG_FULL_BAR3	0x1C
#define REG_FULL_BAR4	0x20
#define REG_FULL_BAR5	0x24

#define REG_WORD_VENDOR	0x00
#define REG_WORD_DEVICE	0x02

#define REG_BYTE_REVISN	0x08
#define REG_BYTE_PROGID	0x09
#define REG_BYTE_SCLASS	0x0A
#define REG_BYTE_CLASS	0x0B
#define REG_BYTE_HEADER	0x0E

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

//static void pci_write(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset, uint32_t data) {
  //  uint32_t lbus = (uint32_t) bus;
  //  uint32_t lslot = (uint32_t) slot;
  //  uint32_t lfunc = (uint32_t) func;
 
//    outl(CONFIG_ADDRESS, (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 0x80000000)));
//    outl(CONFIG_DATA, data);
//}

static char* classes[] = {
    [0] =	"Unspecified Type",
    [1] =	"Mass Storage Controller",
    [2] =	"Network Controller",
    [3] =	"Display Controller",
    [4] =	"Multimedia Controller",
    [5] =	"Memory Controller",
    [6] =	"Bridge Device",
    [7] =	"Simple Communication Controller",
    [8] =	"Base System Peripheral",
    [9] =	"Input Device",
    [10] =	"Docking Station",
    [11] =	"Processor",
    [12] =	"Serial Bus Controller",
    [13] =	"Wireless Controller",
    [14] =	"Intelligent I/O Controller",
    [15] =	"Satellite Communication Controller",
    [16] =	"Encryption/Decryption Controller",
    [17] =	"Data Acquisition and Signal Processing Controller",
    [255] =	"Miscellaneous Device"
};

static void probeDevice(uint8_t bus, uint8_t device);
static void probeBus(uint8_t bus);
static void probeFunction(uint8_t bus, uint8_t device, uint8_t function);
    
static void probeDevice(uint8_t bus, uint8_t device) {
    //__asm__ volatile("INT %0" : "a"(0x80));

    uint8_t function = 0;
    uint16_t vendor = pci_read_word(bus, device, function, REG_WORD_VENDOR);

    if(vendor == 0xFFFF || vendor == 0x0000) return;

    probeFunction(bus, device, function);
    if((pci_read_byte(bus, device, function, REG_BYTE_HEADER) & 0x80) != 0) {
        /* It is a multi-function device, so probe remaining functions */
        for(function = 1; function < 8; function++) {
            if((pci_read_word(bus, device, function, REG_WORD_VENDOR)) != 0xFFFF) {
                probeFunction(bus, device, function);
            }
        }
    }
}

static void probeBus(uint8_t bus) {
    uint8_t device;

    for(device = 0; device < 32; device++) {
        probeDevice(bus, device);
    }
}

static void probeFunction(uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t class = pci_read_byte(bus, device, function, REG_BYTE_CLASS);
    uint8_t subclass = pci_read_byte(bus, device, function, REG_BYTE_SCLASS);

    kprintf("%02X:%02X:%02X %02X:%02X:%02X:%02X %04X:%04X - %s\n",
        bus, device, function, class, subclass,
        pci_read_byte(bus, device, function, REG_BYTE_PROGID),
        pci_read_byte(bus, device, function, REG_BYTE_REVISN),
        pci_read_word(bus, device, function, REG_WORD_VENDOR),
        pci_read_word(bus, device, function, REG_WORD_DEVICE),
        (classes[class] ? classes[class] : "Unknown Type"));

    switch(class) {
        case 0x01:
            switch(class) {
                case 0x01:
                    ide_init(
                        pci_read(bus, device, function, REG_FULL_BAR0),
                        pci_read(bus, device, function, REG_FULL_BAR1),
                        pci_read(bus, device, function, REG_FULL_BAR2),
                        pci_read(bus, device, function, REG_FULL_BAR3),
                        pci_read(bus, device, function, REG_FULL_BAR4));

                    break;
            }
            break;
        case 0x06:
            switch(class) {
                case 0x04:
                    probeBus((pci_read_byte(bus, device, function, 0x19)) & 0xFF);
                    break;
            }
            break;
    }
}

static void probeAllBuses() { 
     if((pci_read_byte(0, 0, 0, REG_BYTE_HEADER) & 0x80) == 0) {
        /* Single PCI host controller */
        probeBus(0);
     } else {
        /* Multiple PCI host controllers */
        for(unsigned int function = 0; function < 8; function++) {
            if(pci_read_word(0, 0, function, REG_WORD_VENDOR) != 0xFFFF) break;
            probeBus(function);
        }
    }
}

void pci_init() {
    outl(CONFIG_ADDRESS, 0x80000000);
    if(inl(CONFIG_ADDRESS) != 0x80000000) {
        panic("PCI not found");
    }

    probeAllBuses();
}

