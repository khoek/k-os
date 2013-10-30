#include "init/initcall.h"
#include "lib/int.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "common/mmio.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "fs/disk.h"
#include "driver/bus/pci.h"
#include "driver/disk/ata.h"
#include "video/log.h"

typedef struct ahci_controller {
    void *base;
} ahci_controller_t;

typedef struct ahci_port {
    uint32_t num;
    ahci_controller_t *cont;

    uint8_t *cmdlist;
    uint32_t cmdlist_phys;

    uint8_t *recvfis;
    uint32_t recvfis_phys;

    uint8_t *cmdtable;
    uint32_t cmdtable_phys;
    
    block_device_t device;
} ahci_port_t;

#define AHCI_DEVICE_PREFIX "sd"

static char * ahci_controller_name(device_t UNUSED(*device)) {
    static int next_id = 0;

    char *name = kmalloc(STRLEN(AHCI_DEVICE_PREFIX) + STRLEN(STR(MAX_UINT)) + 1);
    sprintf(name, "%s%u", AHCI_DEVICE_PREFIX, next_id++);

    return name;
}

static bool ahci_probe(device_t *device) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);
    if(!pci_device->bar[5]) return false;

    ahci_controller_t *cont = device->private = kmalloc(sizeof(ahci_controller_t));

    cont->base = mm_map((void *) pci_device->bar[5]);

    return true;
}

static ssize_t ahci_read(block_device_t *device, void *buff, size_t start, size_t blocks) {
    return -1;
}

static ssize_t ahci_write(block_device_t *device, void *buff, size_t start, size_t blocks) {
    return -1;
}

static block_device_ops_t ahci_device_ops = {
    .read = ahci_read,
    .write = ahci_write,
};

/*static*/ void handle_ahci_irq(interrupt_t *interrupt, ahci_controller_t *cont) {

}

#define AHCI_PORT_BASE 0x100
#define AHCI_PORT_SIZE 0x080

static inline void * port_to_base(ahci_port_t *port) {
    return (void *) (((uint32_t) port->cont->base) + AHCI_PORT_BASE + (port->num * AHCI_PORT_SIZE));
}

#define AHCI_NUM_PORTS 32

#define AHCI_ABAR_PORTS_IMPL 0x0C

#define AHCI_PORT_CMD_ST (1 << 0)
#define AHCI_PORT_CMD_FR (1 << 4)

#define AHCI_PORT_PxCLB  0x00
#define AHCI_PORT_PxCLBU 0x04
#define AHCI_PORT_PxFB   0x08
#define AHCI_PORT_PxFBU  0x0C
#define AHCI_PORT_PxIS   0x10
#define AHCI_PORT_PxIE   0x14
#define AHCI_PORT_PxCMD  0x18
#define AHCI_PORT_PxCI   0x38

static uint8_t buff[ATA_SECTOR_SIZE];
static void sata_identify(ahci_port_t *port) {
    writel(0x00010005, 0x00, port->cmdlist);
    writel(0, 0x04, port->cmdlist);
    writel(port->cmdtable_phys, 0x08, port->cmdlist);
    writel(0, 0x0C, port->cmdlist);
    writel(0, 0x10, port->cmdlist);
    writel(0, 0x14, port->cmdlist);
    writel(0, 0x18, port->cmdlist);
    writel(0, 0x1C, port->cmdlist);

    writel(0x00EC8027, 0x00, port->cmdtable);
    writel(0, 0x04, port->cmdtable);
    writel(0, 0x08, port->cmdtable);
    writel(0, 0x0C, port->cmdtable);
    writel(0, 0x10, port->cmdtable);

    writel(((uint32_t) &buff) - VIRTUAL_BASE, 0x80, port->cmdtable);
    writel(0, 0x84, port->cmdtable);
    writel(0, 0x88, port->cmdtable);
    writel(ATA_SECTOR_SIZE - 1, 0x8C, port->cmdtable);

    writel(0x00, AHCI_PORT_PxIS, port->cont->base);

    void *port_base = port_to_base(port);

    uint32_t tmp = readl(AHCI_PORT_PxCMD, port_base);
    tmp |= AHCI_PORT_CMD_ST;
    tmp |= AHCI_PORT_CMD_FR;
    writel(tmp, AHCI_PORT_PxCMD, port_base);

    writel(1 << port->num, AHCI_PORT_PxCI, port_base);

    while(readl(AHCI_PORT_PxCI, port_base));

    tmp = readl(AHCI_PORT_PxCMD, port_base);
    tmp &= ~AHCI_PORT_CMD_ST;
    tmp &= ~AHCI_PORT_CMD_FR;
    writel(tmp, AHCI_PORT_PxCMD, port_base);

    char model[41];
    for(uint8_t k = 0; k < 40; k += 2) {
        model[k] = buff[ATA_IDENT_MODEL + k + 1];
        model[k + 1] = buff[ATA_IDENT_MODEL + k];
    }
    model[40] = 0;

    uint32_t size = *((uint32_t *) (buff + ATA_IDENT_MAX_LBA_EXT));

    logf("ahci - SATA %s %7uMB", model, size / 1024 / 2);

    port->device.ops = &ahci_device_ops;
    port->device.size = size / ATA_SECTOR_SIZE;
    port->device.block_size = ATA_SECTOR_SIZE;
    
    static uint32_t count = 0;
    char *name = kmalloc(STRLEN(AHCI_DEVICE_PREFIX) + 2);
    memcpy(name, AHCI_DEVICE_PREFIX, STRLEN(AHCI_DEVICE_PREFIX));
    name[STRLEN(AHCI_DEVICE_PREFIX)] = 'a' + count++;
    name[STRLEN(AHCI_DEVICE_PREFIX) + 1] = '\0';
    
    register_block_device(&port->device, name);
    register_disk(&port->device);
}

static void port_init(ahci_controller_t *cont, uint32_t num) {
    ahci_port_t *port = kmalloc(sizeof(ahci_port_t));
    port->cont = cont;
    port->num = num;
    
    page_t *page = alloc_page(0);
    port->cmdlist = (uint8_t *) page_to_virt(page);
    port->cmdlist_phys = (uint32_t) page_to_phys(page);

    page = alloc_page(0);
    port->recvfis = (uint8_t *) page_to_virt(page);
    port->recvfis_phys = (uint32_t) page_to_phys(page);

    page = alloc_pages(14, 0);
    port->cmdtable = (uint8_t *) page_to_virt(page);
    port->cmdtable_phys = (uint32_t) page_to_phys(page);
    
    void *port_base = port_to_base(port);

    writel(port->cmdlist_phys, AHCI_PORT_PxCLB, port_base);
    writel(0, AHCI_PORT_PxCLBU, port_base);
    writel(port->recvfis_phys, AHCI_PORT_PxFB, port_base);
    writel(0, AHCI_PORT_PxFBU, port_base);
    writel(0, AHCI_PORT_PxIS, port_base);
    writel(0, AHCI_PORT_PxIE, port_base);

    sata_identify(port);
}

static void ahci_enable(device_t *device) {
    ahci_controller_t *cont = device->private;

    uint32_t ports_impl = readl(AHCI_ABAR_PORTS_IMPL, cont->base);
    for(uint32_t i = 0; i < AHCI_NUM_PORTS; i++) {
        if(ports_impl & 1) {
            port_init(cont, i);
            break;
        }

        ports_impl >>= 1;
    }
}

static void ahci_disable(device_t UNUSED(*device)) {
    //FIXME stub
}

static void ahci_destroy(device_t UNUSED(*device)) {
    //FIXME stub

    /* Something like this
        free_pages(virt_to_page(cont->cmd_headers), DIV_UP(sizeof(*cont->cmd_headers), PAGE_SIZE));
        free_pages(virt_to_page(cont->fis), DIV_UP(sizeof(*cont->fs), PAGE_SIZE));
        free_pages(virt_to_page(cont->cmd_tables), DIV_UP(sizeof(*cont->cmd_tables), PAGE_SIZE));
    */
}

static pci_ident_t ahci_idents[] = {
    {
        .vendor =     PCI_ID_ANY,
        .device =     PCI_ID_ANY,
        .class  =     0x01060000,
        .class_mask = 0xFFFF0000,
    }
};

static pci_driver_t ahci_driver = {
    .driver = {
        .bus = &pci_bus,

        .name = ahci_controller_name,
        .probe = ahci_probe,

        .enable = ahci_enable,
        .disable = ahci_disable,
        .destroy = ahci_destroy
    },

    .supported = ahci_idents,
    .supported_len = sizeof(ahci_idents) / sizeof(pci_ident_t),
};

static INITCALL ahci_init() {
    register_driver(&ahci_driver.driver);
    return 0;
}

postcore_initcall(ahci_init);
