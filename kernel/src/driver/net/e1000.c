#include "lib/string.h"
#include "lib/printf.h"
#include "common/list.h"
#include "common/compiler.h"
#include "common/math.h"
#include "common/swap.h"
#include "common/init.h"
#include "common/asm.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h" //FIXME sleep(1) should be microseconds not hundredths of a second
#include "device/device.h"
#include "driver/bus/pci.h"
#include "net/interface.h"
#include "net/layer.h"
#include "video/log.h"

//FIXME change these back to -> (PAGE_SIZE / sizeof(rx_desc_t)) after a better kmalloc is implemented
#define NUM_RX_DESCS    128
#define NUM_TX_DESCS    128

//Registers
#define REG_CTRL    0x00000 //Control Register
#define REG_STATUS  0x00008 //Status Register
#define REG_EERD    0x00014 //EEPROM Read Register
#define REG_ICR     0x000C0 //Interrupt Cause Read Register
#define REG_IMS     0x000D0 //Interrupt Mask Set Register
#define REG_RCTL    0x00100 //Recieve Control Register
#define REG_TCTL    0x00400 //Transmit Control Register

#define REG_RDBAL   0x02800 //Recieve Descriptor Base Address Low Register
#define REG_RDBAH   0x02804 //Recieve Descriptor Base Address High Register
#define REG_RDLEN   0x02808 //Recieve Descriptor Ring buff Length (bytes) Register
#define REG_RDH     0x02810 //Recieve Descriptor Head Register
#define REG_RDT     0x02818 //Recieve Descriptor Tail Register

#define REG_TDBAL   0x03800 //Recieve Descriptor Base Address Low Register
#define REG_TDBAH   0x03804 //Recieve Descriptor Base Address High Register
#define REG_TDLEN   0x03808 //Recieve Descriptor Ring buff Length (bytes) Register
#define REG_TDH     0x03810 //Recieve Descriptor Head Register
#define REG_TDT     0x03818 //Recieve Descriptor Tail Register

#define REG_MTA     0x05200 //Multicast Table Array Register
#define REG_RAL     0x05400 //Receive Address Low (MAC filter) Register
#define REG_RAH     0x05404 //Receive Address High (MAC filter) Register

#define REG_LAST    0x1FFFC

//Control Register Flags
#define CTRL_FD     (1 << 0) //Full-Duplex
#define CTRL_ASDE   (1 << 5) //Auto-Speed Detection Enable
#define CTRL_SLU    (1 << 6) //Set Link Up

//Status Flags
#define STATUS_LU   (1 << 1) //Link Up Indication

//EEPROM Register Flags
#define EERD_START  (1 << 0) //Start Read
#define EERD_DONE   (1 << 4) //Read Done

//Recieve Control Register Flags
#define RCTL_EN     (1 << 1)
#define RCTL_SBP    (1 << 2)
#define RCTL_UPE    (1 << 3)
#define RCTL_MPE    (1 << 4)
#define RCTL_LPE    (1 << 5)
#define RCTL_BAM    (1 << 15)
#define RCTL_VFE    (1 << 18)
#define RCTL_CFIEN  (1 << 19)
#define RCTL_CFI    (1 << 20)
#define RCTL_DPF    (1 << 22)
#define RCTL_PMCF   (1 << 23)
#define RCTL_BSEX   (1 << 25)
#define RCTL_SECRC  (1 << 26)

#define RCTL_LBM_MASK       (~(3 << 6))
#define RCTL_LBM_OFF        (0 << 6)
#define RCTL_LBM_ON         (3 << 6)

#define RCTL_RDMTS_MASK     (~(3 << 8))
#define RCTL_RDMTS_HALF     (0 << 8)
#define RCTL_RDMTS_QUATER   (1 << 8)
#define RCTL_RDMTS_EIGHTH   (2 << 8)

#define RCTL_MO_MASK        (~(3 << 12))
#define RCTL_MO_SHIFT_ZERO  (0 << 12)
#define RCTL_MO_SHIFT_ONE   (1 << 12)
#define RCTL_MO_SHIFT_TWO   (2 << 12)
#define RCTL_MO_SHIFT_FOUR  (3 << 12)

#define RCTL_BSIZE_MASK     (~(3 << 16))
#define RCTL_BSIZE_2048     (0 << 16)
#define RCTL_BSIZE_1024     (1 << 16)
#define RCTL_BSIZE_512      (2 << 16)
#define RCTL_BSIZE_256      (3 << 16)
#define RCTL_BSIZE_EX_16384 (1 << 16)
#define RCTL_BSIZE_EX_8192  (2 << 16)
#define RCTL_BSIZE_EX_4096  (3 << 16)

//Transmit Control Register Flags
#define TCTL_EN     (1 << 1)
#define TCTL_PSP    (1 << 3)

//Interrupt Causes
#define ICR_TXDW    (1 << 0)  //Transmit Descriptor Written Back
#define ICR_TXQE    (1 << 1)  //Transmit Queue Empty
#define ICR_LSC     (1 << 2)  //Link Status Change
#define ICR_RXSEQ   (1 << 3)  //Receive Sequence Error
#define ICR_RXDMT   (1 << 4)  //Receive Descriptor Minimum Threshold Reached
#define ICR_RXO     (1 << 6)  //Receiver Overrun
#define ICR_RXT     (1 << 7)  //Receiver Timer Interrupt
#define ICR_MDAC    (1 << 9)  //MDI/O Access Complete
#define ICR_RXCFG   (1 << 10) //Receiving /C/ ordered sets
#define ICR_PHY     (1 << 12) //PHY Interrupt
#define ICR_GPI_ONE (1 << 13) //General Purpose Interrupt (SDP6)
#define ICR_GPI_TWO (1 << 14) //General Purpose Interrupt (SDP7)
#define ICR_TXD_LOW (1 << 15) //Transmit Descriptor Low Threshold Hit
#define ICR_SRPD    (1 << 16) //Small Receive Packet Detected

//RX Descriptor Status Flags
#define RX_DESC_STATUS_DD   (1 << 0) //Descriptor Done
#define RX_DESC_STATUS_EOP  (1 << 1) //End Of Packet

//TX Descriptor Command Flags
#define TX_DESC_CMD_EOP     (1 << 0) //End Of Packet
#define TX_DESC_CMD_IFCS    (1 << 1) //Insert FCS/CRC
#define TX_DESC_CMD_RS      (1 << 3) //Report Status

//TX Descriptor Status Flags
#define TX_DESC_STATUS_DD   (1 << 0) //Descriptor Done

typedef struct rx_desc {
    uint64_t    address;
    uint16_t    length;
    uint16_t    checksum;
    uint8_t     status;
    uint8_t     error;
    uint16_t    special;
} PACKED rx_desc_t;

typedef struct tx_desc {
    uint64_t    address;
    uint16_t    length;
    uint8_t     cso;
    uint8_t     cmd;
    uint8_t     sta;
    uint8_t     css;
    uint16_t    special;
} PACKED tx_desc_t;

typedef struct net_825xx {
    list_head_t list;

    uint32_t mmio, rx_front, tx_front;
    uint8_t *rx_buff[NUM_RX_DESCS];
    uint8_t *tx_buff[NUM_TX_DESCS];
    page_t *rx_page, *tx_page;
    rx_desc_t *rx_desc;
    tx_desc_t *tx_desc;

    net_interface_t interface;
} PACKED net_825xx_t;

static LIST_HEAD(net_825xx_devices);

static uint32_t mmio_read(net_825xx_t *net_device, uint32_t reg) {
    return *(uint32_t *)(net_device->mmio + reg);
}

static void mmio_write(net_825xx_t *net_device, uint32_t reg, uint32_t value) {
     *(uint32_t *)(net_device->mmio + reg) = value;
}

static uint16_t net_eeprom_read(net_825xx_t *net_device, uint8_t addr) {
    mmio_write(net_device, REG_EERD, EERD_START | (((uint32_t) (addr)) << 8));

    uint32_t tmp = 0;
    while(!((tmp = mmio_read(net_device, REG_EERD)) & EERD_DONE))
        sleep(1);

    return (uint16_t) ((tmp >> 16) & 0xFFFF);
}

static void net_825xx_rx_poll(net_interface_t *interface) {
    net_825xx_t *net_device = containerof(interface, net_825xx_t, interface);

    while(net_device->rx_desc[net_device->rx_front].status & RX_DESC_STATUS_DD) {
        if(!(net_device->rx_desc[net_device->rx_front].status & RX_DESC_STATUS_EOP)) {
            logf("825xx - rx: no EOP support, dropping");
        //} else if(net_device->rx_desc[net_device->rx_front].error) {
        //    logf("825xx - rx: descriptor error (0x%X), dropping", net_device->rx_desc[net_device->rx_front].error);
        } else if(net_device->rx_desc[net_device->rx_front].length < 60) {
            logf("825xx - rx: short packet (%u bytes)", net_device->rx_desc[net_device->rx_front].length);
        } else {
            recv_link_eth(interface, net_device->rx_buff[net_device->rx_front], net_device->rx_desc[net_device->rx_front].length);
        }

        net_device->rx_desc[net_device->rx_front].status = 0;
        net_device->rx_front = (net_device->rx_front + 1) % NUM_RX_DESCS;
        mmio_write(net_device, REG_RDT, net_device->rx_front);
    }
}

static void handle_network(interrupt_t UNUSED(*interrupt)) {
    net_825xx_t *net_device;
    LIST_FOR_EACH_ENTRY(net_device, &net_825xx_devices, list) {
        uint32_t icr = mmio_read(net_device, REG_ICR);

        if(icr & ICR_LSC) {
            logf("825xx - link status change! (link is now %s)", mmio_read(net_device, REG_STATUS) & STATUS_LU ? "UP" : "DOWN");
        }

        if(icr & ICR_TXDW) {
            logf("825xx - tx descriptor writeback");
        }

        if(icr & ICR_TXQE) {
            logf("825xx - tx queue empty");
        }

        if(icr & ICR_RXT) {
            net_825xx_rx_poll(&net_device->interface);
        }

        if(icr & ICR_PHY) {
            logf("825xx - PHY int");
        }
    }
}

int32_t net_825xx_tx_send(net_interface_t *net_interface, net_packet_t *packet) {
    net_825xx_t *net_device = containerof(net_interface, net_825xx_t, interface);

    uint8_t *buff = (uint8_t *) net_device->tx_buff[net_device->tx_front];
    uint16_t len = packet->link_len + packet->net_len + packet->tran_len + packet->payload_len;
    memcpy(buff, packet->link_hdr, packet->link_len);
    buff += packet->link_len;
    memcpy(buff, packet->net_hdr, packet->net_len);
    buff += packet->net_len;
    memcpy(buff, packet->tran_hdr, packet->tran_len);
    buff += packet->tran_len;
    memcpy(buff, packet->payload, packet->payload_len);
    buff += packet->payload_len;

    if(len < 60) {
        memset(buff, 0, 60 - len);
        len = 60;
    }

    net_device->tx_desc[net_device->tx_front].length = len;
    net_device->tx_desc[net_device->tx_front].cmd = TX_DESC_CMD_EOP | TX_DESC_CMD_IFCS | TX_DESC_CMD_RS;

    uint32_t old_tx_front = net_device->tx_front;
    net_device->tx_front = (net_device->tx_front + 1) % NUM_TX_DESCS;
    mmio_write(net_device, REG_TDT, net_device->tx_front);

    while(!(net_device->tx_desc[old_tx_front].sta & 0xF))
        sleep(1);

    return net_device->tx_desc[old_tx_front].sta & TX_DESC_STATUS_DD ? 0 : -1;
}

static char * net_825xx_name_prefix = "net_825xx_";

static char * net_825xx_name(device_t UNUSED(*device)) {
    static int next_id = 0;

    char *name = kmalloc(STRLEN(net_825xx_name_prefix) + STRLEN(STR(MAX_UINT)) + 1);
    sprintf(name, "%s%u", net_825xx_name_prefix, next_id++);

    return name;
}

static bool net_825xx_probe(device_t *device) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);

    if(pci_device->bar[0] == 0) {
        logf("825xx - invalid BAR0");
        return false;
    }

    logf("825xx - interface detected");

    net_825xx_t *net_device = pci_device->private = kmalloc(sizeof(net_825xx_t));

    net_device->interface.rx_poll = net_825xx_rx_poll;
    net_device->interface.tx_send = net_825xx_tx_send;

    net_device->mmio = (uint32_t) mm_map((void *) BAR_ADDR_32(pci_device->bar[0]));

    //FIXME this assumes that mm_map will map the pages contiguously, this should not be relied upon!
    for(uint32_t addr = PAGE_SIZE; addr < (DIV_DOWN(REG_LAST, PAGE_SIZE) * PAGE_SIZE); addr += PAGE_SIZE) {
        mm_map((void *) (addr + BAR_ADDR_32(pci_device->bar[0])));
    }

    idt_register(IRQ_OFFSET + pci_device->interrupt, CPL_KERNEL, handle_network);

    ((uint16_t *) net_device->interface.mac.addr)[0] = net_eeprom_read(net_device, 0);
    ((uint16_t *) net_device->interface.mac.addr)[1] = net_eeprom_read(net_device, 1);
    ((uint16_t *) net_device->interface.mac.addr)[2] = net_eeprom_read(net_device, 2);

    if(!(mmio_read(net_device, REG_STATUS) & STATUS_LU)) {
        mmio_write(net_device, REG_CTRL, mmio_read(net_device, REG_CTRL) | CTRL_SLU);
    }

    sleep(1);

    logf("825xx - link is %s", mmio_read(net_device, REG_STATUS) & STATUS_LU ? "UP" : "DOWN");

    for(uint16_t i = 0; i < 128; i++) {
        mmio_write(net_device, REG_MTA + (i * 4), 0);
    }

    //enable all interrupts (and clear existing pending ones)
    mmio_write(net_device, REG_IMS, 0x1F6DC); //this could be 0xFFFFF but that sets some reserved bits
    mmio_read(net_device, REG_ICR);

    //set mac address filter
    mmio_write(net_device, REG_RAL, ((uint16_t *) net_device->interface.mac.addr)[0] | ((((uint32_t)((uint16_t *) net_device->interface.mac.addr)[1]) << 16)));
    mmio_write(net_device, REG_RAH, ((uint16_t *) net_device->interface.mac.addr)[2] | (1 << 31));

    //init RX
    net_device->rx_page = alloc_page(0);
    net_device->rx_desc = (rx_desc_t *) page_to_virt(net_device->rx_page);
    net_device->rx_front = 0;

    for(uint32_t i = 0; i < NUM_RX_DESCS; i++) {
        page_t *page = alloc_page(0);

        net_device->rx_desc[i].address = (uint32_t) page_to_phys(page);
        net_device->rx_desc[i].status = 0;
        net_device->rx_desc[i].error = 0;

        net_device->rx_buff[i] = (uint8_t *) page_to_virt(page);
    }

    mmio_write(net_device, REG_RDBAL, (uint32_t) page_to_phys(net_device->rx_page));
    mmio_write(net_device, REG_RDBAH, 0);
    mmio_write(net_device, REG_RDLEN, NUM_RX_DESCS * sizeof(rx_desc_t));

    mmio_write(net_device, REG_RDH, 0);
    mmio_write(net_device, REG_RDT, NUM_RX_DESCS);

    mmio_write(net_device, REG_RCTL,
        RCTL_BAM            |
        RCTL_BSEX           |
        RCTL_SECRC          |
        RCTL_LPE            |
        RCTL_LBM_OFF        |
        RCTL_RDMTS_HALF     |
        RCTL_MO_SHIFT_ZERO);

    //init TX
    net_device->tx_page = alloc_page(0);
    net_device->tx_desc = (tx_desc_t *) page_to_virt(net_device->tx_page);
    net_device->tx_front = 0;

    for(uint32_t i = 0; i < NUM_TX_DESCS; i++) {
        page_t *page = alloc_page(0);

        net_device->tx_desc[i].address = (uint32_t) page_to_phys(page);
        net_device->tx_desc[i].cmd = 0;

        net_device->tx_buff[i] = (uint8_t *) page_to_virt(page);
    }

    mmio_write(net_device, REG_TDBAL, (uint32_t) page_to_phys(net_device->tx_page));
    mmio_write(net_device, REG_TDBAH, 0);
    mmio_write(net_device, REG_TDLEN, NUM_TX_DESCS * sizeof(tx_desc_t));

    mmio_write(net_device, REG_TDH, 0);
    mmio_write(net_device, REG_TDT, NUM_TX_DESCS);

    return true;
}

static void net_825xx_enable(device_t *device) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);
    net_825xx_t *net_device = pci_device->private;

    list_add(&net_device->list, &net_825xx_devices); //FIXME this is probably not thread-safe

    mmio_write(net_device, REG_TCTL, mmio_read(net_device, REG_TCTL) | TCTL_EN | TCTL_PSP);
    mmio_write(net_device, REG_RCTL, mmio_read(net_device, REG_RCTL) | RCTL_EN);

    register_net_interface(&net_device->interface);
}

static void net_825xx_disable(device_t *device) {
    pci_device_t *pci_device = containerof(device, pci_device_t, device);
    net_825xx_t *net_device = pci_device->private;

    list_rm(&net_device->list); //FIXME this is probably not thread-safe

    mmio_write(net_device, REG_TCTL, mmio_read(net_device, REG_TCTL) & ~(TCTL_EN | TCTL_PSP));
    mmio_write(net_device, REG_RCTL, mmio_read(net_device, REG_RCTL) & ~(RCTL_EN));

    unregister_net_interface(&net_device->interface);
}

static void net_825xx_destroy(device_t UNUSED(*device)) {
    //TODO clean up
}

static pci_ident_t net_825xx_idents[] = {
    {
        .vendor =     0x8086,
        .device =     0x100E,
        .class  =     0x02000000,
        .class_mask = 0xFFFF0000
    }
};

static pci_driver_t net_825xx_driver = {
    .driver = {
        .bus = &pci_bus,

        .name = net_825xx_name,
        .probe = net_825xx_probe,

        .enable = net_825xx_enable,
        .disable = net_825xx_disable,
        .destroy = net_825xx_destroy
    },

    .supported = net_825xx_idents,
    .supported_len = sizeof(net_825xx_idents) / sizeof(pci_ident_t)
};

static INITCALL net_825xx_init() {
    register_driver(&net_825xx_driver.driver);

    return 0;
}

postcore_initcall(net_825xx_init);
