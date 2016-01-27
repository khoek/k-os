#include "lib/int.h"
#include "init/initcall.h"
#include "init/multiboot.h"
#include "common/list.h"
#include "common/compiler.h"
#include "arch/pic.h"
#include "arch/apic.h"
#include "arch/acpi.h"
#include "arch/mp.h"
#include "arch/ioapic.h"
#include "arch/bios.h"
#include "bug/panic.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/clock.h"
#include "sched/sched.h"
#include "sched/proc.h"
#include "log/log.h"

#define BIOS_SEARCH_START 0xF0000
#define BIOS_SEARCH_END   (0xFFFFF + 1)

#define ENTRY_TYPE_PROC   0
#define ENTRY_TYPE_BUS    1
#define ENTRY_TYPE_IOAPIC 2
#define ENTRY_TYPE_IOINT  3
#define ENTRY_TYPE_LINT   4

#define IOAPIC_FLAG_ENABLED (1 << 0)

#define SIG_LEN 4
#define FLOATING_SIG "_MP_"
#define MPCONFIG_SIG "PCMP"

typedef struct mp_floating_header {
    char sig[4];
    uint32_t mp_config_addr;
    uint8_t len;
    uint8_t rev;
    uint8_t checksum;
    uint8_t features[5];
} PACKED mp_floating_header_t;

typedef struct mp_config_header {
    char sig[4];
    uint16_t base_len;
    uint8_t rev;
    uint8_t checksum;
    char oem_id[8];
    char prod_id[12];
    uint32_t oem_table_addr;
    uint16_t oem_table_size;
    uint16_t entry_count;
    uint32_t lapic_addr;
    uint16_t extended_len;
    uint8_t extended_checksum;
    uint8_t reserved;
} PACKED mp_config_header_t;

typedef struct entry_header {
    uint8_t type;
} PACKED entry_header_t;

typedef struct proc_entry {
    uint8_t lapic_id;
    uint8_t lapic_ver;
    uint8_t cpu_flags;
    uint32_t cpu_sig;
    uint32_t feature_flags;
    uint32_t reserved[2];
} PACKED proc_entry_t;

typedef struct bus_entry {
    uint8_t id;
    char type[6];
} PACKED bus_entry_t;

typedef struct ioapic_entry {
    uint8_t id;
    uint8_t ver;
    uint8_t flags;
    uint32_t base_addr;
} PACKED ioapic_entry_t;

typedef struct ioint_entry {
    uint8_t type;
    uint16_t flags;
    uint8_t src_bus_id;
    uint8_t src_bus_irq;
    uint8_t dst_ioapic_id;
    uint8_t dst_ioapic_intin;
} PACKED ioint_entry_t;

typedef struct lint_entry {
    uint8_t type;
    uint16_t flags;
    uint8_t src_bus_id;
    uint8_t src_bus_irq;
    uint8_t dst_lapic_id;
    uint8_t dst_lapic_intin;
} PACKED lint_entry_t;

static const char * int_type_name[] = {
    [0] = "INT",
    [1] = "NMI",
    [2] = "SMI",
    [3] = "ExINT",
};

volatile bool ap_starting = false;
volatile uint32_t next_ap_acpi_id;
volatile void *next_ap_stack;

static bool checksum_valid(void *v, uint32_t len) {
    uint8_t buff = 0;
    uint8_t *data = v;
    for(uint32_t i = 0; i < len; i++) {
        buff += data[i];
    }
    return !buff;
}

void __noreturn mp_ap_start() {
    static volatile uint32_t num = 1;
    processor_t *me = register_proc(num++);
    me->arch.acpi_id = next_ap_acpi_id;
    me->arch.apic_id = apic_get_id();

    ap_starting = false;

    apic_enable();

    sched_loop();
}

//start is a physical addr
static mp_floating_header_t * fhdr_search(uint32_t start, uint32_t end) {
    char *data = map_pages(start, DIV_UP(end - start, PAGE_SIZE));
    uint32_t match_len = 0;
    for(uint32_t off = 0; off < end - start; off++) {
        if(match_len == SIG_LEN) {
            mp_floating_header_t *fhdr = (void *) (data + off - SIG_LEN);

            //Try our best to make sure that this really is the floating header
            if(checksum_valid(fhdr, sizeof(mp_floating_header_t))
                && fhdr->len == 1) {
                return fhdr;
            } else {
                match_len = 0;
            }
        }

        if(data[off] == FLOATING_SIG[match_len]) {
            match_len++;
        } else {
            match_len = 0;
        }
    }

    return NULL;
}

void __init mp_init() {
    uint32_t ebda_addr = bda_getw(BDA_EBDA_ADDR) << 4;
    mp_floating_header_t *fhdr = fhdr_search(ebda_addr, ebda_addr + 0x400);
    if(fhdr == NULL) {
        uint32_t base_end_addr = multiboot_info->mem_lower * 0x400;
        fhdr = fhdr_search(base_end_addr - 0x400, base_end_addr);
    }
    if(fhdr == NULL) {
        fhdr = fhdr_search(BIOS_SEARCH_START, BIOS_SEARCH_END);
    }
    if(fhdr == NULL) {
        panic("mp - floating header not found!");
    }

    kprintf("mp - floating header is at 0x%X", virt_to_phys(fhdr));

    if(!fhdr->mp_config_addr) {
        panic("mp - default configurations not supported!");
    }

    mp_config_header_t *chdr = map_page(fhdr->mp_config_addr);
    map_pages(fhdr->mp_config_addr + PAGE_SIZE, DIV_UP(chdr->base_len, PAGE_SIZE));

    kprintf("mp - configuration header is at 0x%X", fhdr->mp_config_addr);

    if(!checksum_valid(chdr, chdr->base_len)) {
        panic("mp - configuration header has invalid checksum!");
    }

    if(memcmp(chdr->sig, MPCONFIG_SIG, SIG_LEN)) {
        panic("mp - configuration header has invalid signature!");
    }

    char oem_str[9];
    memcpy(oem_str, chdr->oem_id, 8);
    oem_str[8] = '\0';

    char prod_str[17];
    memcpy(prod_str, chdr->prod_id, 16);
    prod_str[16] = '\0';

    kprintf("mp - LAPIC is at 0x%X", chdr->lapic_addr);
    kprintf("mp - oem=%s prod=%s", oem_str, prod_str);
    kprintf("mp - %X entries", chdr->entry_count);

    uint32_t offset = 0;
    uint32_t entries_len = chdr->base_len - sizeof(mp_config_header_t);
    void *entry_base = ((void *) chdr) + sizeof(mp_config_header_t);
    for(uint32_t i = 0; i < chdr->entry_count && offset < entries_len; i++) {
        entry_header_t *ehdr = entry_base + offset;
        offset += sizeof(entry_header_t);

        switch(ehdr->type) {
            case ENTRY_TYPE_PROC: {
                proc_entry_t *proc = entry_base + offset;
                offset += sizeof(proc_entry_t);

                kprintf("mp - proc(%X: v%X): %X:%X:%X:%X:%X", proc->lapic_id, proc->lapic_ver, proc->cpu_flags, proc->cpu_sig, proc->feature_flags, proc->reserved[0], proc->reserved[1]);

                break;
            }
            case ENTRY_TYPE_BUS: {
                bus_entry_t *bus = entry_base + offset;
                offset += sizeof(bus_entry_t);

                kprintf("mp - bus(%X): %c%c%c%c%c%c", bus->id, bus->type[0], bus->type[1], bus->type[2], bus->type[3], bus->type[4], bus->type[5]);

                register_mp_bus(bus->id, bus->type);

                break;
            }
            case ENTRY_TYPE_IOAPIC: {
                ioapic_entry_t *ioapic = entry_base + offset;
                offset += sizeof(ioapic_entry_t);

                kprintf("mp - ioapic(%X:%X v%X) @ %X", ioapic->id, ioapic->flags, ioapic->ver, ioapic->base_addr);

                if(ioapic->flags & IOAPIC_FLAG_ENABLED) {
                    register_ioapic(ioapic->id, ioapic->base_addr);
                }

                break;
            }
            case ENTRY_TYPE_IOINT: {
                ioint_entry_t *ioint = entry_base + offset;
                offset += sizeof(ioint_entry_t);

                kprintf("mp - ioint(%s:%X) %X:%X->%X:%X", int_type_name[ioint->type], ioint->flags, ioint->src_bus_id, ioint->src_bus_irq, ioint->dst_ioapic_id, ioint->dst_ioapic_intin);

                register_ioint(ioint->flags, ioint->src_bus_id, ioint->src_bus_irq, ioint->dst_ioapic_id, ioint->dst_ioapic_intin);

                break;
            }
            case ENTRY_TYPE_LINT: {
                lint_entry_t *lint = entry_base + offset;
                offset += sizeof(lint_entry_t);

                kprintf("mp - lint(%s:%X) %X:%X->%X:%X", int_type_name[lint->type], lint->flags, lint->src_bus_id, lint->src_bus_irq, lint->dst_lapic_id, lint->dst_lapic_intin);

                break;
            }
        }
    }
}
