#include "lib/string.h"
#include "lib/printf.h"
#include "common/asm.h"
#include "sync/spinlock.h"
#include "arch/bios.h"
#include "device/device.h"
#include "mm/mm.h"
#include "fs/char.h"
#include "driver/console/console.h"
#include "driver/console/tty.h"

console_t *con_global = NULL;
static console_t glob;

void console_lockup(console_t *con) {
    con->lockup = true;
}

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

extern __init void vram_early_init(console_t *con);
extern __init void vram_early_remap(console_t *con);

__init void console_early_init() {
    con_global = &glob;
    con_global->lockup = false;
    spinlock_init(&con_global->lock);

    vram_early_init(con_global);
}

__init void console_early_remap() {
    vram_early_remap(con_global);
}

static INITCALL console_init() {
    register_bus(&console_bus, "console");
    register_driver(&console_driver);

    vram_init(con_global);
    keyboard_init(con_global);

    con_global->device.bus = &console_bus;

    return 0;
}

static ssize_t console_char_read(char_device_t UNUSED(*cdev), char *buff, size_t len) {
    return keybuff_read(buff, len);
}

static ssize_t console_char_write(char_device_t *cdev, char *buff, size_t len) {
    console_t *con = cdev->private;
    if(!con->lockup) {
        vram_write(cdev->private, buff, len);
    }

    return len;
}

static char_device_ops_t console_ops = {
    .read = console_char_read,
    .write = console_char_write,
};

static INITCALL console_register() {
    register_device(&con_global->device, &console_bus.node);

    char_device_t *cdev = char_device_alloc();
    cdev->private = con_global;
    cdev->ops = &console_ops;

    register_char_device(cdev, "console");

    return 0;
}

static INITCALL tty_register() {
    tty_create("console");

    return 0;
}

core_initcall(console_init);
subsys_initcall(console_register);
postfs_initcall(tty_register);
