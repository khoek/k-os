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
    logf("%X %X", pci_device->bar[5], cont->base);

    return true;
}

static ssize_t ahci_read(block_device_t *device, void *buff, size_t start, size_t blocks) {
    return -1;
}

static ssize_t ahci_write(block_device_t *device, void *buff, size_t start, size_t blocks) {
    return -1;
}

/*static*/ block_device_ops_t ahci_device_ops = {
    .read = ahci_read,
    .write = ahci_write,
};

/*static*/ void handle_ahci_irq(interrupt_t *interrupt, ahci_controller_t *cont) {

}

#define AHCI_PORT_BASE 0x100
#define AHCI_PORT_SIZE 0x080

static inline void * port_to_base(uint32_t num, void *base) {
    return (void *) (((uint32_t) base) + AHCI_PORT_BASE + (num * AHCI_PORT_SIZE));
}

#define AHCI_NUM_PORTS 32

#define AHCI_ABAR_PORTS_IMPL 0x0C

#define AHCI_PORT_PxCLB  0x00
#define AHCI_PORT_PxCLBU 0x04
#define AHCI_PORT_PxFB   0x08
#define AHCI_PORT_PxFBU  0x0C
#define AHCI_PORT_PxIS   0x10
#define AHCI_PORT_PxIE   0x14
#define AHCI_PORT_PxCMD  0x18
#define AHCI_PORT_PxCI   0x38

static uint8_t *ahci_cmdlist;
static uint32_t ahci_cmdlist_phys;

static uint8_t *ahci_recvfis;
static uint32_t ahci_recvfis_phys;

static uint8_t *ahci_cmdtable;
static uint32_t ahci_cmdtable_phys;

static uint8_t buff[512];
static void sata_identify(void *port_base, void *base) {
    writel(0x00010005, 0x00, ahci_cmdlist);
    writel(0, 0x04, ahci_cmdlist);
    writel(ahci_cmdtable_phys, 0x08, ahci_cmdlist);
    writel(0, 0x0C, ahci_cmdlist);
    writel(0, 0x10, ahci_cmdlist);
    writel(0, 0x14, ahci_cmdlist);
    writel(0, 0x18, ahci_cmdlist);
    writel(0, 0x1C, ahci_cmdlist);

    writel(0x00EC8027, 0x00, ahci_cmdtable);
    writel(0, 0x04, ahci_cmdtable);
    writel(0, 0x08, ahci_cmdtable);
    writel(0, 0x0C, ahci_cmdtable);
    writel(0, 0x10, ahci_cmdtable);

    writel(((uint32_t) &buff) - VIRTUAL_BASE, 0x80, ahci_cmdtable);
    writel(0, 0x84, ahci_cmdtable);
    writel(0, 0x88, ahci_cmdtable);
    writel(0x000001FF, 0x8C, ahci_cmdtable);

    writel(0x00, AHCI_PORT_PxIS, base);

    uint32_t tmp = readl(AHCI_PORT_PxCMD, port_base);
    tmp |= 1 << 0;
    tmp |= 1 << 4;
    writel(tmp, AHCI_PORT_PxCMD, port_base);

    writel(0x00000001, AHCI_PORT_PxCI, port_base);

    while(readl(AHCI_PORT_PxCI, port_base));

    tmp = readl(AHCI_PORT_PxCMD, port_base);
    tmp &= ~(1 << 0);
    tmp &= ~(1 << 4);
    writel(tmp, AHCI_PORT_PxCMD, port_base);

    char model[41];
    for(uint8_t k = 0; k < 40; k += 2) {
        model[k] = buff[ATA_IDENT_MODEL + k + 1];
        model[k + 1] = buff[ATA_IDENT_MODEL + k];
    }
    model[40] = 0;

    uint32_t size = *((uint32_t *) (buff + ATA_IDENT_MAX_LBA_EXT));

    logf("ahci - SATA %s %7uMB", model, size / 1024 / 2);
}

static void port_init(void *port_base, void *base) {
    writel(ahci_cmdlist_phys, AHCI_PORT_PxCLB, port_base);
    writel(0, AHCI_PORT_PxCLBU, port_base);
    writel(ahci_recvfis_phys, AHCI_PORT_PxFB, port_base);
    writel(0, AHCI_PORT_PxFBU, port_base);
    writel(0, AHCI_PORT_PxIS, port_base);
    writel(0, AHCI_PORT_PxIE, port_base);

    sata_identify(port_base, base);
}

static void ahci_enable(device_t *device) {
    ahci_controller_t *cont = device->private;

    uint32_t ports_impl = readl(AHCI_ABAR_PORTS_IMPL, cont->base);
    for(uint32_t i = 0; i < AHCI_NUM_PORTS; i++) {
        if(ports_impl & 1) {
            port_init(port_to_base(i, cont->base), cont->base);
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

    page_t *cmdlist_page = alloc_page(0);
    ahci_cmdlist = (uint8_t *) page_to_virt(cmdlist_page);
    ahci_cmdlist_phys = (uint32_t) page_to_phys(cmdlist_page);

    page_t *recvfis_page = alloc_page(0);
    ahci_recvfis = (uint8_t *) page_to_virt(recvfis_page);
    ahci_recvfis_phys = (uint32_t) page_to_phys(recvfis_page);

    page_t *cmdtable_page = alloc_pages(14, 0);
    ahci_cmdtable = (uint8_t *) page_to_virt(cmdtable_page);
    ahci_cmdtable_phys = (uint32_t) page_to_phys(cmdtable_page);

    return 0;
}

postcore_initcall(ahci_init);
