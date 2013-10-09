#include "lib/int.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "init/initcall.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "driver/bus/pci.h"
#include "video/log.h"

#define AHCI_DEVICE_PREFIX "sd"

#define AHCI_NUM_PRDT_ENTRIES 8

#define AHCI_NUM_PORTS 32
#define AHCI_NUM_CMD_HEADERS 32
#define AHCI_MAX_FIS_SIZE 256

#define AHCI_PORT_STATUS_DET_PRESENT 0x3
#define AHCI_PORT_STATUS_IPM_ACTIVE  0x1

#define TYPE_SATA   0x00000101 // SATA drive
#define TYPE_SATAPI 0xEB140101 // SATAPI drive
#define TYPE_SEMB   0xC33C0101 // Enclosure management bridge
#define TYPE_PM     0x96690101 // Port multiplier

#define PORT_CMD_ST  (1 << 0)
#define PORT_CMD_FRE (1 << 4)
#define PORT_CMD_FR  (1 << 14)
#define PORT_CMD_CR  (1 << 15)

typedef enum dev_type {
    AHCI_DEV_NULL,
    AHCI_DEV_SATA,
    AHCI_DEV_SATAPI,
    AHCI_DEV_SEMB,
    AHCI_DEV_PM,
} dev_type_t;

typedef struct ahci_cmd_header_t {
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

    volatile uint32_t uint8_ts_transferred;

    uint32_t cmd_table_addr_low;
    uint32_t cmd_table_addr_high;

    uint32_t zero2[4];
} PACKED ahci_cmd_header_t;

typedef struct ahci_port {
    uint32_t cmd_list_addr_low;
    uint32_t cmd_list_addr_high;
    uint32_t fis_addr_low;
    uint32_t fis_addr_high;
    uint32_t int_status;
    uint32_t int_enabled;
    uint32_t cmd;
    uint32_t zero1;
    uint32_t task_file;
    uint32_t sig;
    uint32_t sata_status;
    uint32_t sata_control;
    uint32_t sata_error;
    uint32_t sata_active;
    uint32_t command_issue;
    uint32_t sata_notification;
    uint32_t fis_switch;
    uint32_t zero2[0x2C];
    uint32_t vendor[0x10];
} PACKED ahci_port_t;

typedef struct ahci_abar {
    uint32_t capabilities;
    uint32_t control;
    uint32_t int_status;
    uint32_t impl_ports;
    uint32_t version;
    uint32_t ccc_control;
    uint32_t ccc_ports;
    uint32_t em_location;
    uint32_t em_control;
    uint32_t capabilities_ext;
    uint32_t handoff;

    uint8_t zero[0x74];
    uint8_t vendor[0x60];

    ahci_port_t ports[32];
} PACKED ahci_abar_t;

typedef struct ahci_fis_cmd {
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
} ahci_fis_cmd_t;

typedef struct prd {
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t zero1;
    uint32_t dbc:22;
    uint32_t zero2:9;
    uint32_t interrupt:1;
} PACKED prd_t;

typedef struct ahci_cmd_table {
    ahci_fis_cmd_t fis;
    uint8_t atapi_cmd[16];
    uint8_t zero[48];
    prd_t prdt[AHCI_NUM_PRDT_ENTRIES];
} PACKED ahci_cmd_table_t;

typedef struct ahci_device {
    ahci_abar_t *abar;

    ahci_cmd_header_t (*cmd_headers)[AHCI_NUM_PORTS][AHCI_NUM_CMD_HEADERS];
    uint8_t (*fis)[AHCI_NUM_PORTS][AHCI_MAX_FIS_SIZE];
    ahci_cmd_table_t (*cmd_tables)[AHCI_NUM_PORTS];
} ahci_device_t;

void enable_port(ahci_port_t *port) {
    while(port->cmd & PORT_CMD_CR);

    port->cmd |= PORT_CMD_FRE;
    port->cmd |= PORT_CMD_ST;
}

void disable_port(ahci_port_t *port) {
    port->cmd &= ~PORT_CMD_ST;
    port->cmd &= ~PORT_CMD_FRE;

    while(port->cmd & PORT_CMD_FR || port->cmd & PORT_CMD_CR);
}

#define VIRT_TO_PHYS(v) ((typeof(v)) virt_to_phys(v))

void setup_port(ahci_device_t *device, int num) {
    disable_port(&device->abar->ports[num]);

    ahci_port_t *port = &device->abar->ports[num];

    port->cmd_list_addr_low = (uint32_t) &(*VIRT_TO_PHYS(device->cmd_headers))[num];
    port->cmd_list_addr_high = 0;

    port->fis_addr_low = (uint32_t) &(*VIRT_TO_PHYS(device->fis))[num];
    port->fis_addr_high = 0;

    ahci_cmd_header_t *header = (*device->cmd_headers)[num];
    for(uint32_t i = 0; i < AHCI_NUM_CMD_HEADERS; i++) {
        header[i].prdt_length = AHCI_NUM_PRDT_ENTRIES;

        header[i].cmd_table_addr_low = (uint32_t) &VIRT_TO_PHYS(device->cmd_tables)[num][i];
        header[i].cmd_table_addr_high = 0;

        memset(&device->cmd_tables[num][i], 0, sizeof(device->cmd_tables[num][i]));
    }

    enable_port(&device->abar->ports[num]);
}

static dev_type_t check_type(ahci_port_t *port) {
     if ((port->sata_status & 0x0F) != AHCI_PORT_STATUS_DET_PRESENT)
          return AHCI_DEV_NULL;
     if (((port->sata_status >> 8) & 0x0F) != AHCI_PORT_STATUS_IPM_ACTIVE)
          return AHCI_DEV_NULL;

     switch (port->sig) {
          case TYPE_SATAPI:
                return AHCI_DEV_SATAPI;
          case TYPE_SEMB:
                return AHCI_DEV_SEMB;
          case TYPE_PM:
                return AHCI_DEV_PM;
          default:
                return AHCI_DEV_SATA;
     }
}

static char * ahci_controller_name(device_t UNUSED(*device)) {
    static int next_id = 0;

    char *name = kmalloc(STRLEN(AHCI_DEVICE_PREFIX) + STRLEN(STR(MAX_UINT)) + 1);
    sprintf(name, "%s%u", AHCI_DEVICE_PREFIX, next_id++);

    return name;
}

static bool ahci_probe(device_t *device) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);
    if(!pci_device->bar[5]) return false;

    ahci_device_t *cont = kmalloc(sizeof(ahci_device_t));
    cont->abar = (ahci_abar_t *) mm_map((void *) BAR_ADDR_32(pci_device->bar[5]));

    if(!cont->abar->impl_ports) {
        goto probe_bar_fail;
    }

    cont->cmd_headers = page_to_virt(alloc_pages(DIV_UP(sizeof(*cont->cmd_headers), PAGE_SIZE), 0));
    cont->fis = page_to_virt(alloc_pages(DIV_UP(sizeof(*cont->fis), PAGE_SIZE), 0));
    cont->cmd_tables = page_to_virt(alloc_pages(DIV_UP(sizeof(*cont->cmd_tables), PAGE_SIZE), 0));

    memset(cont->cmd_headers, 0, sizeof(*cont->cmd_headers));
    memset(cont->fis, 0, sizeof(*cont->fis));
    memset(cont->cmd_tables, 0, sizeof(*cont->cmd_tables));

    device->private = cont;

    return true;

probe_bar_fail:
    //TODO implementation pending mm_unmap(cont->abar);
    kfree(cont, sizeof(ahci_device_t));

    return false;
}

static void ahci_enable(device_t *device) {
    ahci_device_t *cont = device->private;
    for (int i = 0; i < AHCI_NUM_PORTS; i++) {
        if (cont->abar->impl_ports & (1 << i)) {
            switch(check_type(&cont->abar->ports[i])) {
                case AHCI_DEV_SATA:
                    logf("ahci - port %u: SATA", i);
                    break;
                case AHCI_DEV_SATAPI:
                    logf("ahci - port %u: SATAPI", i);
                    break;
                case AHCI_DEV_SEMB:
                    logf("ahci - port %u: SEMB", i);
                    break;
                case AHCI_DEV_PM:
                    logf("ahci - port %u: PM", i);
                    break;
                default:
                    continue;
            }

            setup_port(cont, i);
        }
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
