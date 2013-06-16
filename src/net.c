#include "init.h"
#include "net.h"
#include "io.h"
#include "mm.h"
#include "pci.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h" //FIXME sleep(1) should be microseconds not hundredths of a second
#include "log.h"

#define REG_CTRL    0x00000 //Control register
#define REG_STATUS  0x00008 //Status register
#define REG_EERD    0x00014 //EEPROM Read register
#define REG_ICR     0x000C0 //Interrupt Cause Read register
#define REG_IMS     0x000D0 //Interrupt Mask Set register
#define REG_MTA     0x05200 //Multicast Table Array register
#define REG_RAL     0x05400 //Receive Address Low (MAC filter)
#define REG_RAH     0x05404 //Receive Address High (MAC filter)

#define REG_LAST    0x1FFFC

#define CTRL_FD     (1 << 0) //Full-Duplex
#define CTRL_ASDE   (1 << 5) //Auto-Speed Detection Enable
#define CTRL_SLU    (1 << 6) //Set Link Up

#define STATUS_LU   (1 << 1) //Link Up Indication

static uint8_t mac[6];
//static uint32_t rx_count, tx_count, packets_dropped;
static uint32_t mmio;

static uint32_t mmio_read(uint32_t reg) {
    return *(uint32_t *)(mmio + reg);
}

static void mmio_write(uint32_t reg, uint32_t value) {
     *(uint32_t *)(mmio + reg) = value;
}

static uint16_t net_eeprom_read(uint8_t addr) {
    mmio_write(REG_EERD, 1 | ((uint32_t)(addr) << 8));
    
    uint32_t tmp = 0;
    while(!((tmp = mmio_read(REG_EERD)) & (1 << 4)))
        sleep(1);
        
    return (uint16_t) ((tmp >> 16) & 0xFFFF);
}

static void handle_network(interrupt_t UNUSED(*interrupt)) {
    logf("net int!!!");
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
        
    mmio_write(REG_CTRL, mmio_read(REG_CTRL) | CTRL_SLU);
    
    logf("net - link is %s", mmio_read(REG_STATUS) & STATUS_LU ? "UP" : "DOWN");
    
    for(uint16_t i = 0; i < 128; i++) {
        mmio_write(REG_MTA + (i * 4), 0);
    }
        
    //enable all interrupts (and clear existing pending ones)
    mmio_write(REG_IMS, 0x1F6DC); //this could be a 0xFFFFF but that sets some reserved bits
    mmio_read(REG_ICR);
    
    //TODO enable RX and TX
    mmio_write(REG_RAL, ((uint16_t *) mac)[0] | ((((uint32_t)((uint16_t *) mac)[1]) << 16) & 0xFFFF));
    mmio_write(REG_RAH, ((uint16_t *) mac)[2] | (1 << 31));
}
