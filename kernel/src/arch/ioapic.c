#include "common/list.h"
#include "lib/string.h"
#include "bug/panic.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "arch/ioapic.h"
#include "log/log.h"

#define REGSEL_OFF 0x00
#define REGWIN_OFF 0x10

#define REG_IOREDTBL 0x10
#define REDTBL_ENTRY_LEN 0x2

#define BUS_NAME_ISA "ISA   "
#define BUS_NAME_PCI "PCI   "

#define RFLAG_MASK (1 << 8)
#define RFLAG_TRIG_LEVEL (1 << 7)
#define RFLAG_TRIG_EDGE (0 << 7)
#define RFLAG_ACTIVE_LO (1 << 5)
#define RFLAG_ACTIVE_HI (0 << 5)
#define RFLAG_DSTMODE_PHYS (0 << 3)
#define RFLAG_DSTMODE_LOGI (1 << 3)
#define RFLAG_DELMODE_FIXED (0 << 0)
#define RFLAG_DELMODE_LOWEST (1 << 0)

#define IOINT_POLARITY_MASK (3 << 0)
#define IOINT_TRIGGER_MASK  (3 << 2)

#define POLARITY_DEFAULT 0x0
#define POLARITY_HIGH 0x1
#define POLARITY_LOW 0x3

#define TRIGGER_DEFAULT 0x0
#define TRIGGER_EDGE 0x1
#define TRIGGER_LEVEL 0x3

#define INTIN_NUM 24

static uint8_t next_pci_vec = 0x40;

typedef struct mp_bus {
    uint8_t id;
    char name[6];
    list_head_t list;
} mp_bus_t;

typedef struct ioapic {
    uint8_t id;
    uint32_t base_addr;
    uint8_t intinvecs[24];
    list_head_t list;
} ioapic_t;

typedef struct pci_ioint {
    ioapic_t *ioapic;
    uint8_t dev_num;
    uint8_t intin;
    uint8_t flags;
    list_head_t list;
} pci_ioint_t;

static DEFINE_LIST(bus_list);
static DEFINE_LIST(ioapic_list);
static DEFINE_LIST(pci_ioint_list);

static inline void writel(ioapic_t *ioapic, uint32_t reg, uint32_t val) {
    *(volatile uint32_t *)(ioapic->base_addr + REGSEL_OFF) = reg;
    *(volatile uint32_t *)(ioapic->base_addr + REGWIN_OFF) = val;
}

// static inline uint32_t readl(ioapic_t *ioapic, uint32_t reg) {
//     *(volatile uint32_t *)(ioapic->base_addr + REGSEL_OFF) = reg;
//     return *(volatile uint32_t *)(ioapic->base_addr + REGWIN_OFF);
// }

static void set_redirect(ioapic_t *ioapic, uint8_t intin, uint8_t apic_id, uint8_t flags, uint8_t vec) {
    uint32_t off = REG_IOREDTBL + REDTBL_ENTRY_LEN * intin;
    writel(ioapic, off + 1, ((uint32_t) apic_id) << 24);
    writel(ioapic, off + 0, (((uint32_t) flags) << 8) | vec);
}

static mp_bus_t * find_mp_bus(uint8_t id) {
    mp_bus_t *mp_bus;
    LIST_FOR_EACH_ENTRY(mp_bus, &bus_list, list) {
        if(mp_bus->id == id) return mp_bus;
    }

    return NULL;
}

static ioapic_t * find_ioapic(uint8_t id) {
    ioapic_t *ioapic;
    LIST_FOR_EACH_ENTRY(ioapic, &ioapic_list, list) {
        if(ioapic->id == id) return ioapic;
    }

    return NULL;
}

void register_ioapic(uint8_t id, uint32_t base_addr) {
    ioapic_t *ioapic = kmalloc(sizeof(ioapic_t));
    ioapic->id = id;
    ioapic->base_addr = (uint32_t) map_page((void *) base_addr);
    for(uint32_t i = 0; i < INTIN_NUM; i++) {
        ioapic->intinvecs[i] = 0;
    }

    list_add(&ioapic->list, &ioapic_list);
}

void register_mp_bus(uint8_t id, char name[6]) {
    mp_bus_t *bus = kmalloc(sizeof(mp_bus_t));
    bus->id = id;
    memcpy(&bus->name, name, 6);

    list_add(&bus->list, &bus_list);
}

static void register_pci_ioint(ioapic_t *ioapic, uint8_t bus_irq, uint8_t intin, uint16_t ioint_flags) {
    pci_ioint_t *pioint = kmalloc(sizeof(pci_ioint_t));
    pioint->ioapic = ioapic;
    pioint->dev_num = bus_irq >> 2;
    pioint->intin = intin;
    pioint->flags = ioint_flags;

    list_add(&pioint->list, &pci_ioint_list);
}

static uint8_t get_pci_rflags(uint8_t ioint_flags) {
    uint8_t rflags = RFLAG_DSTMODE_PHYS | RFLAG_DELMODE_FIXED;

    switch(ioint_flags & IOINT_POLARITY_MASK) {
        case POLARITY_DEFAULT:
        case POLARITY_LOW: {
            rflags |= RFLAG_ACTIVE_LO;
            break;
        }
        case POLARITY_HIGH: {
            rflags |= RFLAG_ACTIVE_HI;
            break;
        }
    }

    switch(ioint_flags & IOINT_TRIGGER_MASK) {
        case TRIGGER_DEFAULT:
        case TRIGGER_LEVEL: {
            rflags |= RFLAG_TRIG_LEVEL;
            break;
        }
        case TRIGGER_EDGE: {
            rflags |= RFLAG_TRIG_EDGE;
            break;
        }
    }

    return rflags;
}

static uint8_t get_isa_rflags(uint8_t ioint_flags) {
    uint8_t rflags = RFLAG_DSTMODE_PHYS | RFLAG_DELMODE_FIXED;

    switch(ioint_flags & IOINT_POLARITY_MASK) {
        case POLARITY_LOW: {
            rflags |= RFLAG_ACTIVE_LO;
            break;
        }
        case POLARITY_DEFAULT:
        case POLARITY_HIGH: {
            rflags |= RFLAG_ACTIVE_HI;
            break;
        }
    }

    switch(ioint_flags & IOINT_TRIGGER_MASK) {
        case TRIGGER_LEVEL: {
            rflags |= RFLAG_TRIG_LEVEL;
            break;
        }
        case TRIGGER_DEFAULT:
        case TRIGGER_EDGE: {
            rflags |= RFLAG_TRIG_EDGE;
            break;
        }
    }

    return rflags;
}

static void register_isa_ioint(ioapic_t *ioapic, uint8_t bus_irq, uint8_t intin, uint16_t ioint_flags) {
    set_redirect(ioapic, intin, 0, get_isa_rflags(ioint_flags), bus_irq + IRQ_OFFSET);
}

void register_ioint(uint16_t flags, uint8_t bus_id, uint8_t bus_irq, uint8_t ioapic_id, uint8_t intin) {
    mp_bus_t *mp_bus = find_mp_bus(bus_id);

    if(!mp_bus) {
        panicf("mp - invalid mp bus id %X", bus_id);
    }

    if(!strcmp(BUS_NAME_PCI, mp_bus->name)) {
        register_pci_ioint(find_ioapic(ioapic_id), bus_irq, intin, flags);
    } else if(!strcmp(BUS_NAME_ISA, mp_bus->name)) {
        register_isa_ioint(find_ioapic(ioapic_id), bus_irq, intin, flags);
    }
}

uint8_t find_pci_ioint(uint8_t dev_num) {
    pci_ioint_t *pioint;
    LIST_FOR_EACH_ENTRY(pioint, &pci_ioint_list, list) {
        if(pioint->dev_num == dev_num) {
            uint8_t *vec = &pioint->ioapic->intinvecs[pioint->intin];
            if(!*vec) {
                *vec = next_pci_vec++;

                set_redirect(pioint->ioapic, pioint->intin, 0, get_pci_rflags(pioint->flags), *vec);
            }
            return *vec;
        }
    }

    return 0;
}
