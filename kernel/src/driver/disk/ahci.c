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

#define AHCI_DEVICE_PREFIX "sd"

#define AHCI_NUM_PORTS 32
#define AHCI_NUM_PRDT_ENTRIES 16

#define AHCI_ABAR_PORTS_IMPL 0x0C

#define PORT_CMD_ST (1 << 0)
#define PORT_CMD_FR (1 << 4)

#define PORT_REG_PxCLB  0x00
#define PORT_REG_PxCLBU 0x04
#define PORT_REG_PxFB   0x08
#define PORT_REG_PxFBU  0x0C
#define PORT_REG_PxIS   0x10
#define PORT_REG_PxIE   0x14
#define PORT_REG_PxCMD  0x18
#define PORT_REG_PxTFD  0x20
#define PORT_REG_PxSIG  0x24
#define PORT_REG_PxSSTS 0x28
#define PORT_REG_PxSCTL 0x2C
#define PORT_REG_PxSERR 0x30
#define PORT_REG_PxSACT 0x34
#define PORT_REG_PxCI   0x38

#define PORT_SIG_SATA   0x00000101
#define PORT_SIG_SATAPI 0xEB140101
#define PORT_SIG_SEMB   0xC33C0101
#define PORT_SIG_PM     0x96690101

#define PORT_STATUS_DET_PRESENT 0x3

#define PORT_STATUS_IPM_ACTIVE 0x1

enum {
    FIS_TYPE_H2D = 0x27,
    FIS_TYPE_D2H = 0x34,
    FIS_TYPE_DMA_ACT = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS = 0xA1,
};

typedef enum ahci_port_type {
    PORT_TYPE_NONE,
    PORT_TYPE_SATA,
    PORT_TYPE_SATAPI,
} ahci_port_type_t;

typedef struct ahci_controller {
    void *base;
} ahci_controller_t;

typedef struct ahci_cmd_fis {
    uint8_t type;

    uint8_t port_multiplier:4;
    uint8_t zero1:3;
    uint8_t is_command:1;

    uint8_t command;
    uint8_t feature_low;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high;

    uint8_t count_low;
    uint8_t count_high;
    uint8_t icc;
    uint8_t control;

    uint8_t zero2[4];
} ahci_cmd_fis_t;

typedef volatile struct ahci_port_cmdlist {
    uint8_t fis_length:5;
    uint8_t atapi:1;
    uint8_t write:1;
    uint8_t prefetchable:1;

    uint8_t reset:1;
    uint8_t bist:1;
    uint8_t clear_busy_on_ok:1;
    uint8_t zero1:1;
    uint8_t port_multiplier_port:4;

    uint16_t prdt_length;

    uint32_t uint8_ts_transferred;

    uint32_t cmdtable_addr_low;
    uint32_t cmdtable_addr_high;

    uint32_t zero2[4];
} PACKED ahci_cmdlist_t;

typedef struct prd {
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t zero;
    uint32_t bytes;
} PACKED prd_t;

typedef volatile struct ahci_port_cmdtable {
    ahci_cmd_fis_t fis;
    uint8_t atapi_cmd[16];
    uint8_t zero[92];
    prd_t prdt[AHCI_NUM_PRDT_ENTRIES];
} PACKED ahci_cmdtable_t;

typedef struct ahci_port {
    ahci_port_type_t type;
    uint32_t num;
    ahci_controller_t *cont;

    ahci_cmdlist_t *cmdlist;
    uint32_t cmdlist_phys;

    uint8_t *recvfis;
    uint32_t recvfis_phys;

    ahci_cmdtable_t *cmdtable;
    uint32_t cmdtable_phys;

    block_device_t device;
} ahci_port_t;

#define AHCI_PORT_BASE 0x100
#define AHCI_PORT_SIZE 0x080

static inline void * port_to_base(ahci_port_t *port) {
    return (void *) (((uint32_t) port->cont->base) + AHCI_PORT_BASE + (port->num * AHCI_PORT_SIZE));
}

static ssize_t sata_access(bool write, ahci_port_t *port, void *buff, size_t lba, size_t count) {
    void *port_base = port_to_base(port);

    memset((void *) port->cmdlist, 0, sizeof(ahci_cmdlist_t));
    port->cmdlist->fis_length = sizeof(ahci_cmd_fis_t) / sizeof(uint32_t);
    port->cmdlist->cmdtable_addr_low = port->cmdtable_phys;
    port->cmdlist->cmdtable_addr_high = 0;

    memset((void *) &port->cmdtable->fis, 0, sizeof(ahci_cmd_fis_t));
    port->cmdtable->fis.type = FIS_TYPE_H2D;
    port->cmdtable->fis.is_command = true;
    port->cmdtable->fis.command = write ? ATA_CMD_DMA_WRITE_EXT : ATA_CMD_DMA_READ_EXT;

    port->cmdtable->fis.lba0 = (lba >> 0) & 0xFF;
	port->cmdtable->fis.lba1 = (lba >> 8) & 0xFF;
	port->cmdtable->fis.lba2 = (lba >> 16) & 0xFF;
	port->cmdtable->fis.lba3 = (lba >> 24) & 0xFF;
	port->cmdtable->fis.lba4 = /*(lba >> 32) & 0xFF*/ 0;
	port->cmdtable->fis.lba5 = /*(lba >> 40) & 0xFF*/ 0;

	port->cmdtable->fis.device = 1 << 6;

	port->cmdtable->fis.count_low = count & 0xFF;
	port->cmdtable->fis.count_high = (count >> 8) & 0xFF;

    uint32_t addr = (uint32_t) virt_to_phys(buff);
    uint32_t i;
    for(i = 0; i < AHCI_NUM_PRDT_ENTRIES - 1 && count >= 16; i++) {
        memset((void *) &port->cmdtable->prdt[i], 0, sizeof(prd_t));
		port->cmdtable->prdt[i].addr_low = addr;
		port->cmdtable->prdt[i].bytes = 16 * ATA_SECTOR_SIZE;

		addr += 16 * ATA_SECTOR_SIZE;
		count -= 16;
	}

	if(count) {
        memset((void *) &port->cmdtable->prdt[i], 0, sizeof(prd_t));
	    port->cmdtable->prdt[i].addr_low = addr;
	    port->cmdtable->prdt[i].bytes = count * ATA_SECTOR_SIZE;

        port->cmdlist->prdt_length = i + 1;
	} else {
        port->cmdlist->prdt_length = i;
	}

    writel(port->cont->base, PORT_REG_PxIS, 0);

    uint32_t tmp = readl(port_base, PORT_REG_PxCMD);
    tmp |= PORT_CMD_ST;
    tmp |= PORT_CMD_FR;
    writel(port_base, PORT_REG_PxCMD, tmp);

    writel(port_base, PORT_REG_PxCI, 1);
    while(readl(port_base, PORT_REG_PxCI));

    tmp = readl(port_base, PORT_REG_PxCMD);
    tmp &= ~PORT_CMD_ST;
    tmp &= ~PORT_CMD_FR;
    writel(port_base, PORT_REG_PxCMD, tmp);

    return 1;
}

static ssize_t ahci_read(block_device_t *device, void *buff, size_t start, size_t blocks) {
    ahci_port_t *port = containerof(device, ahci_port_t, device);
    if(port->type == PORT_TYPE_SATA) {
        return sata_access(false, port, buff, start, blocks);
    } else if(port->type == PORT_TYPE_SATAPI) {
        //TODO implement SATAPI
        return -1;
    }

    return -1;
}

static ssize_t ahci_write(block_device_t *device, void *buff, size_t start, size_t blocks) {
    ahci_port_t *port = containerof(device, ahci_port_t, device);
    if(port->type == PORT_TYPE_SATA) {
        return sata_access(true, port, buff, start, blocks);
    } else if(port->type == PORT_TYPE_SATAPI) {
        //TODO implement SATAPI
        return -1;
    }

    return -1;
}

static block_device_ops_t ahci_device_ops = {
    .read = ahci_read,
    .write = ahci_write,
};

static void sata_identify(ahci_port_t *port) {
    port->type = PORT_TYPE_SATA;

    void *port_base = port_to_base(port);
    uint8_t *buff = kmalloc(ATA_SECTOR_SIZE);

    memset((void *) port->cmdlist, 0, sizeof(ahci_cmdlist_t));
    port->cmdlist->fis_length = sizeof(ahci_cmd_fis_t) / sizeof(uint32_t);
    port->cmdlist->prdt_length = 1;
    port->cmdlist->cmdtable_addr_low = port->cmdtable_phys;
    port->cmdlist->cmdtable_addr_high = 0;

    memset((void *) &port->cmdtable->fis, 0, sizeof(ahci_cmd_fis_t));
    port->cmdtable->fis.type = FIS_TYPE_H2D;
    port->cmdtable->fis.is_command = true;
    port->cmdtable->fis.command = ATA_CMD_IDENTIFY;

    memset((void *) &port->cmdtable->prdt[0], 0, sizeof(prd_t));
    port->cmdtable->prdt[0].addr_low = (uint32_t) virt_to_phys(buff);
    port->cmdtable->prdt[0].bytes = ATA_SECTOR_SIZE - 1;

    writel(port->cont->base, PORT_REG_PxIS, 0);

    uint32_t tmp = readl(port_base, PORT_REG_PxCMD);
    tmp |= PORT_CMD_ST;
    tmp |= PORT_CMD_FR;
    writel(port_base, PORT_REG_PxCMD, tmp);

    writel(port_base, PORT_REG_PxCI, 1);
    while(readl(port_base, PORT_REG_PxCI));

    tmp = readl(port_base, PORT_REG_PxCMD);
    tmp &= ~PORT_CMD_ST;
    tmp &= ~PORT_CMD_FR;
    writel(port_base, PORT_REG_PxCMD, tmp);

    char model[ATA_MODEL_LENGTH + 1];
    for(uint8_t k = 0; k < ATA_MODEL_LENGTH; k += 2) {
        model[k] = buff[ATA_IDENT_MODEL + k + 1];
        model[k + 1] = buff[ATA_IDENT_MODEL + k];
    }
    model[ATA_MODEL_LENGTH] = 0;

    uint32_t size = *((uint32_t *) (buff + ATA_IDENT_MAX_LBA_EXT));

    logf("ahci - SATA   %s %7uMB", model, size / 1024 / 2);

    memset(&port->device, 0, sizeof(block_device_t));
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

static char * ahci_controller_name(device_t UNUSED(*device)) {
    static int next_id = 0;

    char *name = kmalloc(STRLEN(AHCI_DEVICE_PREFIX) + STRLEN(XSTR(MAX_UINT)) + 1);
    sprintf(name, "%s%u", AHCI_DEVICE_PREFIX, next_id++);

    return name;
}

static bool ahci_probe(device_t *device) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);
    if(!pci_device->bar[5]) return false;

    ahci_controller_t *cont = device->private = kmalloc(sizeof(ahci_controller_t));

    cont->base = map_page((void *) pci_device->bar[5]);

    return true;
}

/*
static void handle_ahci_irq(interrupt_t *interrupt, ahci_controller_t *cont) {

}
*/

static void port_init(ahci_controller_t *cont, uint32_t num) {
    ahci_port_t *port = kmalloc(sizeof(ahci_port_t));
    port->cont = cont;
    port->num = num;
    port->type = PORT_TYPE_NONE;

    page_t *page = alloc_pages(DIV_UP(sizeof(ahci_cmdlist_t), PAGE_SIZE), 0);
    port->cmdlist = (ahci_cmdlist_t *) page_to_virt(page);
    port->cmdlist_phys = (uint32_t) page_to_phys(page);

    page = alloc_page(0);
    port->recvfis = (uint8_t *) page_to_virt(page);
    port->recvfis_phys = (uint32_t) page_to_phys(page);

    page = alloc_pages(DIV_UP(sizeof(ahci_cmdtable_t), PAGE_SIZE), 0);
    port->cmdtable = (ahci_cmdtable_t *) page_to_virt(page);
    port->cmdtable_phys = (uint32_t) page_to_phys(page);

    void *port_base = port_to_base(port);

    writel(port_base, PORT_REG_PxCLB , port->cmdlist_phys);
    writel(port_base, PORT_REG_PxCLBU, 0);
    writel(port_base, PORT_REG_PxFB  , port->recvfis_phys);
    writel(port_base, PORT_REG_PxFBU , 0);
    writel(port_base, PORT_REG_PxIS  , 0);
    writel(port_base, PORT_REG_PxIE  , 0);

    //Is the port unused?
    if ((readl(port_base, PORT_REG_PxSSTS) & 0x0F) != PORT_STATUS_DET_PRESENT) {
        return;
    }

    //Is the port powered down?
    if (((readl(port_base, PORT_REG_PxSSTS) >> 8) & 0x0F) != PORT_STATUS_IPM_ACTIVE) {
        return;
    }

    switch (readl(port_base, PORT_REG_PxSIG)) {
        case PORT_SIG_SATAPI:
            logf("ahci - SATAPI unsupported");
            break;
        case PORT_SIG_SEMB:
            logf("ahci - SEMB unsupported");
            break;
        case PORT_SIG_PM:
            logf("ahci - PM unsupported");
            break;
        default:
            sata_identify(port);
            break;
    }
}

static void ahci_enable(device_t *device) {
    ahci_controller_t *cont = device->private;

    uint32_t ports_impl = readl(cont->base, AHCI_ABAR_PORTS_IMPL);
    for(uint32_t i = 0; i < AHCI_NUM_PORTS; i++) {
        if(ports_impl & 1) {
            port_init(cont, i);
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
