#include "lib/int.h"
#include "common/mmio.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/pic.h"
#include "arch/apic.h"
#include "mm/mm.h"
#include "sched/sched.h"
#include "time/clock.h"
#include "video/log.h"

#define TIMER_VECTOR    0x7E

#define REG_ID          0x020
#define REG_TASK_PRIO   0x080
#define REG_EOI         0x0B0
#define REG_LDR         0x0D0
#define REG_DFR         0x0E0
#define REG_SPURIOUS    0x0F0
#define REG_ICR_LOW     0x300
#define REG_ICR_HIGH    0x310

#define REG_TIMER_INITIAL   0x380
#define REG_TIMER_CURRENT   0x390
#define REG_TIMER_DIVIDE    0x3E0

#define REG_LVT_TIMER   0x320
#define REG_LVT_LINT0   0x350
#define REG_LVT_LINT1   0x360

#define TIMER_MODE_PERIODIC (1 << 17)

#define APIC_MASTER_ENABLE  (1 << 8)
#define APIC_DISABLE        (1 << 16)
#define APIC_NMI            (0x4 << 8)

#define CMD_FLAG_PENDING (1 << 12)

static void *apic_base;

uint32_t apic_get_id() {
    return readl(apic_base, REG_ID) >> 24;
}

void apic_issue_command(uint8_t target_id, uint16_t type, uint32_t flags, uint8_t vector) {
    writel(apic_base, REG_ICR_HIGH, target_id << 24);
    writel(apic_base, REG_ICR_LOW , (type << 8) | flags | vector);

    while(readl(apic_base, REG_ICR_LOW) & CMD_FLAG_PENDING);
}

static clock_event_source_t apic_clock_event_source = {
    .name = "apic",
    .rating = 5,

    .freq = 0, //FIXME
};

static bool apic_is_spurious(uint32_t vector) {
    return vector == 0xFF;
}

static void apic_eoi() {
    writel(apic_base, REG_EOI, 0);
}

static void handle_timer() {
    apic_clock_event_source.event(&apic_clock_event_source);
}

void __init apic_enable() {
    writel(apic_base, REG_DFR, 0xFFFFFFFF);
    writel(apic_base, REG_LDR, (readl(apic_base, REG_LDR) & 0x00FFFFFF) | 1);
    writel(apic_base, REG_LVT_TIMER, TIMER_MODE_PERIODIC | TIMER_VECTOR);
    writel(apic_base, REG_LVT_LINT0, APIC_DISABLE);
    writel(apic_base, REG_LVT_LINT1, APIC_DISABLE);
    writel(apic_base, REG_TASK_PRIO, 0);

    writel(apic_base, REG_TIMER_DIVIDE, 1);
    writel(apic_base, REG_TIMER_INITIAL, 16000000);
    writel(apic_base, REG_SPURIOUS, APIC_MASTER_ENABLE | 0xFF);
}

void __init apic_init(void *base) {
    apic_base = map_page(base);
    eoi_handler = apic_eoi;
    is_spurious = apic_is_spurious;

    pic_configure(0xFF, 0xFF);

    apic_enable();

    register_isr(TIMER_VECTOR, CPL_KRNL, handle_timer, NULL);
    register_clock_event_source(&apic_clock_event_source);

    sti();
}
