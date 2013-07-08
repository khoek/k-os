#include "lib/printf.h"
#include "common/compiler.h"
#include "common/init.h"
#include "common/asm.h"
#include "bug/debug.h"
#include "bug/panic.h"
#include "mm/cache.h"
#include "device/device.h"
#include "driver/bus/pci.h"
#include "video/log.h"

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
#define REG_BYTE_PROGIF 0x09
#define REG_BYTE_SCLASS 0x0A
#define REG_BYTE_CLASS  0x0B
#define REG_BYTE_HEADER 0x0E
#define REG_BYTE_INTRPT 0x3C

//Header type 0x01
#define REG_BYTE_2NDBUS 0x19

static char * classes[] = {
    [0x00] = "Unspecified Type",
    [0x01] = "Mass Storage Controller",
    [0x02] = "Network Controller",
    [0x03] = "Display Controller",
    [0x04] = "Multimedia Controller",
    [0x05] = "Memory Controller",
    [0x06] = "Bridge Device",
    [0x07] = "Simple Communication Controller",
    [0x08] = "Base System Peripheral",
    [0x09] = "Input Device",
    [0x0A] = "Docking Station",
    [0x0B] = "Processor",
    [0x0C] = "Serial Bus Controller",
    [0x0D] = "Wireless Controller",
    [0x0E] = "Intelligent I/O Controller",
    [0x0F] = "Satellite Communication Controller",
    [0x10] = "Encryption/Decryption Controller",
    [0x11] = "Data Acquisition and Signal Processing Controller",
    [0xFF] = "Miscellaneous Device"
};

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

static bool pci_match(device_t *device, driver_t *driver) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);
    pci_driver_t *pci_driver = containerof(driver, pci_driver_t, driver);

    for(uint32_t i = 0; i < pci_driver->supported_len; i++)  {
        pci_ident_t *ident = &pci_driver->supported[i];

        if((pci_device->ident.vendor == ident->vendor || ident->vendor == PCI_ID_ANY)
            && (pci_device->ident.device == ident->device || ident->device == PCI_ID_ANY)
            && !((pci_device->ident.class ^ ident->class) & ident->class_mask)) {
            return true;
        }
    }

    return false;
}

bus_t pci_bus = {
    .match = pci_match
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
    pci_device_t *dev = kmalloc(sizeof(pci_device_t));

    dev->loc.bus = bus;
    dev->loc.device = device;
    dev->loc.function = function;

    dev->device.bus = &pci_bus;

    dev->ident.vendor = pci_read_word(bus, device, function, REG_WORD_VENDOR);
    dev->ident.device = pci_read_word(bus, device, function, REG_WORD_DEVICE);
    dev->ident.class = (((uint32_t) pci_read_byte(bus, device, function, REG_BYTE_CLASS)) << 24)
                    | (((uint32_t) pci_read_byte(bus, device, function, REG_BYTE_SCLASS)) << 16)
                    | (((uint32_t) pci_read_byte(bus, device, function, REG_BYTE_PROGIF)) << 8)
                    | (((uint32_t) pci_read_byte(bus, device, function, REG_BYTE_REVISN)));

    dev->bar[0] = pci_read(bus, device, function, REG_FULL_BAR0);
    dev->bar[1] = pci_read(bus, device, function, REG_FULL_BAR1);
    dev->bar[2] = pci_read(bus, device, function, REG_FULL_BAR2);
    dev->bar[3] = pci_read(bus, device, function, REG_FULL_BAR3);
    dev->bar[4] = pci_read(bus, device, function, REG_FULL_BAR4);
    dev->bar[5] = pci_read(bus, device, function, REG_FULL_BAR5);
    dev->interrupt = pci_read_byte(bus, device, function, REG_BYTE_INTRPT);

    logf("pci - %02X:%02X:%02X %08X %04X:%04X - %s",
        bus, device, function, dev->ident.class, dev->ident.vendor, dev->ident.device,
        (classes[(dev->ident.class >> 24)] ? classes[(dev->ident.class >> 24)] : "Unknown Type"));

    register_device(&dev->device, &pci_bus.node);
}

static char * bridge_name_prefix = "bridge";

static char * bridge_name(device_t UNUSED(*device)) {
    static int next_id = 0;

    char *name = kmalloc(STRLEN(bridge_name_prefix) + STRLEN(STR(MAX_UINT)) + 1);
    sprintf(name, "%s%u", bridge_name_prefix, next_id++);

    return name;
}

static bool bridge_probe(device_t *UNUSED(device)) {
    return true;
}

static void bridge_enable(device_t *device) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);

    probe_bus(pci_read_byte(pci_device->loc.bus, pci_device->loc.device, pci_device->loc.function, REG_BYTE_2NDBUS));
}

static void bridge_disable(device_t UNUSED(*device)) {
}

static void bridge_destroy(device_t UNUSED(*device)) {
}

static pci_ident_t bridge_idents[] = {
    {
        .vendor =     PCI_ID_ANY,
        .device =     PCI_ID_ANY,
        .class  =     0x06040000,
        .class_mask = 0xFFFF0000
    }
};

static pci_driver_t bridge_driver = {
    .driver = {
        .bus = &pci_bus,

        .name = bridge_name,
        .probe = bridge_probe,

        .enable = bridge_enable,
        .disable = bridge_disable,
        .destroy = bridge_destroy
    },

    .supported = bridge_idents,
    .supported_len = sizeof(bridge_idents) / sizeof(pci_ident_t)
};

static INITCALL pci_init() {
    register_bus(&pci_bus, "pci_bus");
    register_driver(&bridge_driver.driver);

    return 0;
}

static INITCALL pci_probe() {
    outl(CONFIG_ADDRESS, 0x80000000);
    if(inl(CONFIG_ADDRESS) == 0x80000000) { //does PCI exist?
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
    }

    return 0;
}

core_initcall(pci_init);
subsys_initcall(pci_probe);
