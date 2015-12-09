#include <stdarg.h>

#include "lib/string.h"
#include "lib/printf.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "arch/bios.h"
#include "device/device.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "driver/console/console.h"

#define BUFFSIZE 1024
#define BIOS_PORT_BASE 0x463

static console_t *primary = NULL;

static bool console_match(device_t *device, driver_t *driver) {
    return true;
}

static bus_t console_bus = {
    .match = console_match
};

static char * console_name_prefix = "console";

static char * console_name(device_t UNUSED(*device)) {
    static int next_id = 0;

    char *name = kmalloc(STRLEN(console_name_prefix) + STRLEN(STR(MAX_UINT)) + 1);
    sprintf(name, "%s%u", console_name_prefix, next_id++);

    return name;
}

static bool console_probe(device_t *device) {
    console_t *con = containerof(device, console_t, device);
    con->row = 0;
    con->col = 0;
    con->color = 0x07;

    return true;
}

static void console_enable(device_t UNUSED(*device)) {
}

static void console_disable(device_t UNUSED(*device)) {
}

static void console_destroy(device_t UNUSED(*device)) {
}

static driver_t console_driver = {
    .bus = &console_bus,

    .name = console_name,
    .probe = console_probe,

    .enable = console_enable,
    .disable = console_disable,
    .destroy = console_destroy
};

void console_color(console_t *con, const char c) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    con->color = c;

    spin_unlock_irqstore(&con->lock, flags);
}

void console_clear(console_t *con) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    for(uint32_t i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        con->vram[i * 2] = ' ';
        con->vram[i * 2 + 1] = 0x7;
    }
    con->col = 0;
    con->row = 0;

    spin_unlock_irqstore(&con->lock, flags);
}

void console_write(console_t *con, const char *str, const uint8_t len) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    for (int i = 0; i < len; i++) {
        if(str[i] == '\n') {
            con->col = 0;
            con->row++;
        } else if(str[i] == '\r') {
            con->col = 0;
        } else {
            con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2] = str[i];
            con->vram[(con->row * CONSOLE_WIDTH + con->col) * 2 + 1] = con->color;
            con->col++;
        }

        if(con->col == CONSOLE_WIDTH) {
            con->col = 0;
            con->row++;
        }

        if(con->row == CONSOLE_HEIGHT) {
            memmove(con->vram, con->vram + CONSOLE_WIDTH * 2, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2);

            for(int i = 0; i < 80; i++) {
                (con->vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2] = ' ';
                (con->vram + (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * 2)[i * 2 + 1] = 7;
            }

            con->row = CONSOLE_HEIGHT - 1;
        }
    }

    spin_unlock_irqstore(&con->lock, flags);

    console_cursor(con, con->row, con->col);
}

void console_cursor(console_t *con, const uint8_t r, const uint8_t c) {
    uint32_t flags;
    spin_lock_irqsave(&con->lock, &flags);

    if (r >= CONSOLE_HEIGHT || c >= CONSOLE_WIDTH) {
        con->row = 0;
        con->col = 0;
    } else {
        con->row = r;
        con->col = c;
    }

    outb(con->port, 0x0e);
    outb(con->port + 1, ((con->row * CONSOLE_WIDTH + con->col) >> 8) & 0xff);

    outb(con->port, 0x0f);
    outb(con->port + 1, (con->row * CONSOLE_WIDTH + con->col) & 0xff);

    spin_unlock_irqstore(&con->lock, flags);
}

void console_puts(console_t *con, const char* str) {
    console_write(con, str, strlen(str));
}

//FIXME remove the need for this
static char buff[BUFFSIZE];
DEFINE_SPINLOCK(buffer_lock);
void console_putsf(console_t *con, const char* fmt, ...) {
    uint32_t flags;
    spin_lock_irqsave(&buffer_lock, &flags);

    va_list va;
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);

    console_puts(con, buff);

    spin_unlock_irqstore(&buffer_lock, flags);
}

console_t * console_primary() {
    return primary;
}

static INITCALL console_init() {
    register_bus(&console_bus, "console");
    register_driver(&console_driver);

    return 0;
}

static INITCALL console_register() {
    primary = kmalloc(sizeof(console_t));
    primary->vram = map_page((void *) BIOS_VRAM);
    spinlock_init(&primary->lock);

    uint16_t *bios_port_data = map_page((void *) BIOS_PORT_BASE);
    primary->port = *bios_port_data;
    //TODO unmap bios_port_data page

    primary->device.bus = &console_bus;

    console_clear(primary);

    register_device(&primary->device, &console_bus.node);

    return 0;
}

core_initcall(console_init);
subsys_initcall(console_register);
