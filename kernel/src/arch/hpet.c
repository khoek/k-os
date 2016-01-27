#include "common/mmio.h"
#include "common/list.h"
#include "arch/acpi.h"
#include "arch/hpet.h"
#include "mm/mm.h"
#include "time/clock.h"
#include "log/log.h"

#define REG_CAPS 0x00
#define REG_CONF 0x10
#define REG_MCR  0xF0

#define REG_TIMER_BASE(num) (0x100 + (0x20 * num))
#define REG_TIMER_CAPS(num) (REG_TIMER_BASE(num) + 0x00)

typedef struct hpet_caps {
    uint8_t rev;
    uint8_t num_timers:5;
    bool long_counter:1;
    bool zero:1;
    bool legacy_capable:1;
    uint16_t vendor;
    uint32_t period;
} PACKED hpet_caps_t;

static void *base;

static union {
    hpet_caps_t reg;
    uint64_t raw;
} caps;

static uint64_t hpet_read()  {
    return readq(base, REG_MCR);
}

static clock_t hpet_clock = {
    .name = "HPET",
    .rating = 10,

    .read = hpet_read,
};

void __init hpet_init(acpi_sdt_t *hpet) {
    static bool once = false;
    if(once) return;
    once = true;

    hpet_data_t *data = (void *) hpet->data;

    base = map_page((uint32_t) data->base.addr);
    caps.raw = readq(base, REG_CAPS);

    kprintf("hpet - there are %u timers", caps.reg.num_timers + 1);

    writeq(base, REG_CONF, 0);
    writeq(base, REG_MCR , 0);
    writeq(base, REG_CONF, 1);

    hpet_clock.freq = FEMPTOS_PER_SEC / caps.reg.period;

    register_clock(&hpet_clock);
}
