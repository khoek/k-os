#include <stdint.h>
#include <string.h>
#include "ahci.h"
#include "console.h"

#define AHCI_BASE   0x400000 // 4M

#define AHCI_DEV_NULL   0
#define AHCI_DEV_SATA   1
#define AHCI_DEV_SATAPI 2
#define AHCI_DEV_SEMB   3
#define AHCI_DEV_PM     4

#define AHCI_FLAG_ST    (1)
#define AHCI_FLAG_FRE   (1 << 4)
#define AHCI_FLAG_FR    (1 << 14)
#define AHCI_FLAG_CR    (1 << 15)

#define AHCI_PORT_STATUS_DET_PRESENT 0x3
#define AHCI_PORT_STATUS_IPM_ACTIVE 0x1

#define TYPE_SATA   0x00000101 // SATA drive
#define TYPE_SATAPI 0xEB140101 // SATAPI drive
#define TYPE_SEMB   0xC33C0101 // Enclosure management bridge
#define TYPE_PM     0x96690101 // Port multiplier
 
typedef struct {
	uint32_t	dba;		// Data base address
	uint32_t	dbau;		// Data base address upper 32 bits
	uint32_t	rsv0;		// Reserved
 
	// DW3
	uint32_t	dbc:22;		// uint8_t count, 4M max
	uint32_t	rsv1:9;		// Reserved
	uint32_t	i:1;		// Interrupt on completion
} ahci_prdt_entry;

typedef struct {
	// 0x00
	uint8_t	cfis[64];	// Command FIS
 
	// 0x40
	uint8_t	acmd[16];	// ATAPI command, 12 or 16 uint8_ts
 
	// 0x50
	uint8_t	rsv[48];	// Reserved
 
	// 0x80
	ahci_prdt_entry	prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} achi_cmd_table;

typedef struct {
	// DW0
	uint8_t	cfl:5;		// Command FIS length in uint32_tS, 2 ~ 16
	uint8_t	a:1;		// ATAPI
	uint8_t	w:1;		// Write, 1: H2D, 0: D2H
	uint8_t	p:1;		// Prefetchable
 
	uint8_t	r:1;		// Reset
	uint8_t	b:1;		// BIST
	uint8_t	c:1;		// Clear busy upon R_OK
	uint8_t	rsv0:1;		// Reserved
	uint8_t	pmp:4;		// Port multiplier port
 
	uint16_t	prdtl;		// Physical region descriptor table length in entries
 
	// DW1
	volatile
	uint32_t	prdbc;		// Physical region descriptor uint8_t count transferred
 
	// DW2, 3
	uint32_t	ctba;		// Command table descriptor base address
	uint32_t	ctbau;		// Command table descriptor base address upper 32 bits
 
	// DW4 - 7
	uint32_t	rsv1[4];	// Reserved
} ahci_cmd_header;
 
typedef struct {
    uint32_t    clb;        // 0x00, command list base address, 1K-uint8_t aligned
    uint32_t    clbu;        // 0x04, command list base address upper 32 bits
    uint32_t    fb;        // 0x08, FIS base address, 256-uint8_t aligned
    uint32_t    fbu;        // 0x0C, FIS base address upper 32 bits
    uint32_t    is;        // 0x10, interrupt status
    uint32_t    ie;        // 0x14, interrupt enable

    uint32_t    cmd;        // 0x18, command and status

    uint32_t    rsv0;        // 0x1C, Reserved

    uint32_t    tfd;        // 0x20, task file data
    uint32_t    type;        // 0x24, signature/type
    uint32_t    sstatus;        // 0x28, SATA status (SCR0:SStatus)
    uint32_t    sctl;        // 0x2C, SATA control (SCR2:SControl)
    uint32_t    serr;        // 0x30, SATA error (SCR1:SError)
    uint32_t    sact;        // 0x34, SATA active (SCR3:SActive)
    uint32_t    ci;        // 0x38, command issue
    uint32_t    sntf;        // 0x3C, SATA notification (SCR4:SNotification)
    uint32_t    fbs;        // 0x40, FIS-based switch control

    uint32_t    rsv1[11];    // 0x44 ~ 0x6F, Reserved
    uint32_t    vendor[4];    // 0x70 ~ 0x7F, vendor specific
} ahci_port;

typedef struct {
    // 0x00 - 0x2B, Generic Host Control
    uint32_t    caps;    // 0x00, Host capabilities
    uint32_t    ghc;        // 0x04, Global host control
    uint32_t    is;        // 0x08, Interrupt status
    uint32_t    ports;        // 0x0C, Ports implemented
    uint32_t    version;    // 0x10, Version
    uint32_t    ccc_ctl;    // 0x14, Command completion coalescing control
    uint32_t    ccc_pts;    // 0x18, Command completion coalescing ports
    uint32_t    em_loc;        // 0x1C, Enclosure management location
    uint32_t    em_ctl;        // 0x20, Enclosure management control
    uint32_t    caps2;        // 0x24, Host capabilities extended
    uint32_t    bohc;        // 0x28, BIOS/OS handoff control and status
 
    // 0x2C - 0x9F, Reserved
    uint8_t     rsv[0xA0-0x2C];
 
    // 0xA0 - 0xFF, Vendor specific registers
    uint8_t     vendor[0x100-0xA0];
 
    // 0x100 - 0x10FF, Port control registers
    ahci_port   port[32];    // 1 ~ 32
} achi_abar;

static void start_port(ahci_port *port) {
    // Wait until CR (bit15) is cleared
    //while (port->cmd & AHCI_FLAG_CR);
 
    // Set FRE (bit4) and ST (bit0)
    port->cmd |= AHCI_FLAG_FRE;
    port->cmd |= AHCI_FLAG_ST; 
}
 
static void stop_port(ahci_port *port) {
kprintf("port %X ", port);

    // Clear ST (bit0)
    port->cmd &= ~AHCI_FLAG_ST;
 
kprintf("%u %u %u", port->cmd, port->cmd & AHCI_FLAG_FR, port->cmd & AHCI_FLAG_CR);
    // Wait until FR (bit14), CR (bit15) are cleared
    //while((port->cmd & AHCI_FLAG_FR) /*|| (port->cmd & AHCI_FLAG_CR)*/);
 
    // Clear FRE (bit4)
    port->cmd &= ~AHCI_FLAG_FRE;
}
 
static void port_rebase(ahci_port *port, int portno) {
kprintf("im here");
    stop_port(port);

kprintf("stopped");
 
    // Command list offset: 1K * port
    // Command list entry size = 32
    // Command list entry maxim count = 32
    // Command list maxim size = 32*32 = 1K per port
    port->clb = AHCI_BASE + (portno << 10);
    port->clbu = 0;
    memset((void*)(port->clb), 0, 1024);
 
kprintf("s2");
    // FIS offset: 32K+256*portno
    // FIS entry size = 256 uint8_ts per port
    port->fb = AHCI_BASE + (32<<10) + (portno<<8);
    port->fbu = 0;
    memset((void*)(port->fb), 0, 256);
 
kprintf("s2");
    // Command table offset: 40K + 8K*portno
    // Command table size = 256*32 = 8K per port
    ahci_cmd_header *cmdheader = (ahci_cmd_header*)(port->clb);
    for (int i = 0; i < 32; i++) {
        cmdheader[i].prdtl = 8;    // 8 prdt entries per command table
                    // 256 uint8_ts per command table, 64+16+48+16*8
        // Command table offset: 40K + 8K*portno + cmdheader_index*256
        cmdheader[i].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
        cmdheader[i].ctbau = 0;
        memset((void*)cmdheader[i].ctba, 0, 256);
    }
 
kprintf("s3");
    start_port(port);
kprintf("s4");
}

static uint32_t check_type(ahci_port *port) {  
    if ((port->sstatus & 0x0F) != AHCI_PORT_STATUS_DET_PRESENT) // Check drive status
        return AHCI_DEV_NULL;
    if (((port->sstatus >> 8) & 0x0F) != AHCI_PORT_STATUS_IPM_ACTIVE)
        return AHCI_DEV_NULL;
 
    switch (port->type) {
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

void ahci_init(void *BAR5) {
    achi_abar *abar = (achi_abar *) BAR5;

    for (int i = 0; i < 32; i++) {
        if (abar->ports & (1 << i)) {
            switch (check_type(&abar->port[i])) {
                case AHCI_DEV_SATA:
                    kprintf("SATA drive found at port %d\n", i);
                    break;
                case AHCI_DEV_SATAPI:
                    kprintf("SATAPI drive found at port %d\n", i);
                    break;
                case AHCI_DEV_SEMB:
                    kprintf("SEMB drive found at port %d\n", i);
                    break;
                case AHCI_DEV_PM:
                    kprintf("PM drive found at port %d\n", i);
                    break;
                default:
                    kprintf("No drive found at port %d\n", i);
                    continue;
            }

kprintf("invoking");
            port_rebase(&abar->port[i], i);
        }
    }
}
