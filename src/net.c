#include "common.h"
#include "init.h"
#include "string.h"
#include "net.h"
#include "io.h"
#include "mm.h"
#include "pci.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h" //FIXME sleep(1) should be microseconds not hundredths of a second
#include "ethernet.h"
#include "log.h"

#define NUM_RX_DESCS    (PAGE_SIZE / sizeof(rx_desc_t))
#define NUM_TX_DESCS    (PAGE_SIZE / sizeof(tx_desc_t))

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

static uint8_t mac[6];
static uint32_t mmio, rx_front, tx_front;
static uint8_t *rx_buff[NUM_RX_DESCS];
//static uint8_t *tx_buff[NUM_TX_DESCS];
static page_t *rx_page, *tx_page;
static rx_desc_t *rx_desc;
static tx_desc_t *tx_desc;

static uint32_t mmio_read(uint32_t reg) {
    return *(uint32_t *)(mmio + reg);
}

static void mmio_write(uint32_t reg, uint32_t value) {
     *(uint32_t *)(mmio + reg) = value;
}

static uint16_t net_eeprom_read(uint8_t addr) {
    mmio_write(REG_EERD, EERD_START | (((uint32_t) (addr)) << 8));

    uint32_t tmp = 0;
    while(!((tmp = mmio_read(REG_EERD)) & EERD_DONE))
        sleep(1);

    return (uint16_t) ((tmp >> 16) & 0xFFFF);
}

static void net_rx_poll() {
    while(rx_desc[rx_front].status & RX_DESC_STATUS_DD) {
        if(!(rx_desc[rx_front].status & RX_DESC_STATUS_EOP)) {
            logf("net - rx: no EOP support, dropping");
        } else if(rx_desc[rx_front].error) {
            logf("net - rx: descriptor error (0x%X), dropping", rx_desc[rx_front].error);
        } else if(rx_desc[rx_front].length < 60) {
            logf("net - rx: short packet (%u bytes)", rx_desc[rx_front].length);
        } else {
            ethernet_handle(rx_buff[rx_front], rx_desc[rx_front].length);
        }

        rx_desc[rx_front].status = 0;
        rx_front = (rx_front + 1) % NUM_RX_DESCS;
		mmio_write(REG_RDT, rx_front);
    }
}

static void handle_network(interrupt_t UNUSED(*interrupt)) {
    uint32_t icr = mmio_read(REG_ICR);

    if(icr & ICR_LSC) {
        logf("net - link status change! (device is now %s)", mmio_read(REG_STATUS) & STATUS_LU ? "UP" : "DOWN");
    }

    if(icr & ICR_TXDW) {
        logf("net - tx descriptor writeback");
    }

    if(icr & ICR_TXQE) {
        logf("net - tx queue empty");
    }

    if(icr & ICR_RXT) {
        net_rx_poll();
    }

    if(icr & ICR_PHY) {
        logf("net - PHY int");
    }
}

int32_t net_send(void *packet, uint16_t length) {
	tx_desc[tx_front].address = (uint32_t) packet;
	tx_desc[tx_front].length = length;
	tx_desc[tx_front].cmd = TX_DESC_CMD_EOP | TX_DESC_CMD_IFCS | TX_DESC_CMD_RS;

	uint32_t old_tx_front = tx_front;

	tx_front = (tx_front + 1) % NUM_TX_DESCS;
	mmio_write(REG_TDT, tx_front);

	while(!(tx_desc[old_tx_front].sta & 0xF))
	    sleep(1);

	return tx_desc[old_tx_front].sta & TX_DESC_STATUS_DD ? 0 : -1;
}

void __init net_825xx_init(pci_device_t dev) {
    if(dev.bar[0] == 0) {
        logf("net: 825xx - invalid BAR0");
        return;
    }

    logf("net - 825xx interface detected");

    mmio = (uint32_t) mm_map(BAR_ADDR_32(dev.bar[0]));

    //FIXME this assumes that mm_map will map the pages configuously, this should not be relied upon!
    for(uint32_t addr = PAGE_SIZE; addr < (DIV_DOWN(REG_LAST, PAGE_SIZE) * PAGE_SIZE); addr += PAGE_SIZE) {
        mm_map(addr + BAR_ADDR_32(dev.bar[0]));
    }

    idt_register(IRQ_OFFSET + dev.interrupt, CPL_KERNEL, handle_network);

    ((uint16_t *) mac)[0] = net_eeprom_read(0);
    ((uint16_t *) mac)[1] = net_eeprom_read(1);
    ((uint16_t *) mac)[2] = net_eeprom_read(2);

    logf("net - MAC %X:%X:%X:%X:%X:%X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if(!(mmio_read(REG_STATUS) & STATUS_LU)) {
        mmio_write(REG_CTRL, mmio_read(REG_CTRL) | CTRL_SLU);
    }

    sleep(1);

    logf("net - link is %s", mmio_read(REG_STATUS) & STATUS_LU ? "UP" : "DOWN");

    for(uint16_t i = 0; i < 128; i++) {
        mmio_write(REG_MTA + (i * 4), 0);
    }

    //enable all interrupts (and clear existing pending ones)
    mmio_write(REG_IMS, 0x1F6DC); //this could be a 0xFFFFF but that sets some reserved bits
    mmio_read(REG_ICR);

    //set mac address filter
    mmio_write(REG_RAL, ((uint16_t *) mac)[0] | ((((uint32_t)((uint16_t *) mac)[1]) << 16) & 0xFFFF));
    mmio_write(REG_RAH, ((uint16_t *) mac)[2] | (1 << 31));

    //init RX
    rx_page = alloc_page(0);
    rx_desc = (rx_desc_t *) page_to_virt(rx_page);
    rx_front = 0;

    for(uint32_t i = 0; i < NUM_RX_DESCS; i++) {
        page_t *page = alloc_page(0);

        rx_desc[i].address = (uint32_t) page_to_phys(page);
        rx_desc[i].status = 0;

        rx_buff[i] = (uint8_t *) page_to_virt(page);
    }

    mmio_write(REG_RDBAL, (uint32_t) page_to_phys(rx_page));
    mmio_write(REG_RDBAH, 0);
    mmio_write(REG_RDLEN, NUM_RX_DESCS * sizeof(rx_desc_t));

	mmio_write(REG_RDH, 0);
	mmio_write(REG_RDT, NUM_RX_DESCS);

	mmio_write(REG_RCTL,
	RCTL_BAM            |
	RCTL_BSEX           |
	RCTL_SECRC          |
	RCTL_LPE            |
	RCTL_LBM_OFF        |
	RCTL_RDMTS_HALF     |
	RCTL_MO_SHIFT_ZERO);

    //init TX
    tx_page = alloc_page(0);
    tx_desc = (tx_desc_t *) page_to_virt(tx_page);
    tx_front = 0;

    for(uint32_t i = 0; i < NUM_RX_DESCS; i++) {
        tx_desc[i].address = 0;
        tx_desc[i].cmd = 0;
    }

    mmio_write(REG_TDBAL, (uint32_t) page_to_phys(tx_page));
    mmio_write(REG_TDBAH, 0);
    mmio_write(REG_TDLEN, NUM_TX_DESCS * sizeof(tx_desc_t));

	mmio_write(REG_TDH, 0);
	mmio_write(REG_TDT, NUM_TX_DESCS);

    //enable tx then rx
    mmio_write(REG_TCTL, mmio_read(REG_TCTL) | TCTL_EN | TCTL_PSP);
	mmio_write(REG_RCTL, mmio_read(REG_RCTL) | RCTL_EN);
}
