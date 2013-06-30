#include <stddef.h>

#include "int.h"
#include "string.h"
#include "printf.h"
#include "debug.h"
#include "list.h"
#include "init.h"
#include "device.h"
#include "cache.h"
#include "log.h"

static node_t root = {
    .name = "/",
    .parent = NULL,
    .children = LIST_MAKE_HEAD(root.children)
};

static LIST_HEAD(buses);

static uint32_t next_id = 0;
static const char *generic_name = "dev";

static inline void node_init(node_t *node) {
    list_init(&node->children);
}

static inline void node_add(node_t *child, node_t *parent) {
    list_add(&child->list, &parent->children);
    child->parent = parent;
}

void register_bus(bus_t *bus, char *name) {
    BUG_ON(!bus->match);

    bus->node.name = name;
    node_init(&bus->node);
    node_add(&bus->node, &root);

    list_init(&bus->drivers);
    list_init(&bus->devices);
    list_init(&bus->unident_devices);

    list_add(&bus->list, &buses);
}

void register_driver(driver_t *driver) {
    BUG_ON(!driver->bus);
    BUG_ON(!driver->name);
    BUG_ON(!driver->probe);
    BUG_ON(!driver->enable);
    BUG_ON(!driver->disable);
    BUG_ON(!driver->destroy);

    list_add(&driver->list, &driver->bus->drivers);

    bus_t *bus;
    LIST_FOR_EACH_ENTRY(bus, &buses, list) {
        device_t *device;
        LIST_FOR_EACH_ENTRY(device, &bus->unident_devices, list) {
            if(device->bus->match(device, driver) && driver->probe(device)) {
                device->driver = driver;
                device->node.name = device->driver->name(device);
                list_move(&device->list, &device->bus->devices);

                driver->enable(device);
            }
        }
    }
}

void register_device(device_t *device, node_t *parent) {
    BUG_ON(!device->bus);

    node_init(&device->node);
    node_add(&device->node, parent);

    device->driver = NULL;

    driver_t *driver;
    LIST_FOR_EACH_ENTRY(driver, &device->bus->drivers, list) {
        if(device->bus->match(device, driver) && driver->probe(device)) {
            device->driver = driver;
            device->node.name = device->driver->name(device);
            list_add(&device->list, &device->bus->devices);

            driver->enable(device);
        }
    }

    if(!device->driver) {
        device->node.name = kmalloc(STRLEN(generic_name) + STRLEN(STR(MAX_UINT)) + 1);
        sprintf(device->node.name, "%s%u", generic_name, next_id++);

        list_add(&device->list, &device->bus->unident_devices);
    }
}

static void traverse_log(node_t *node, uint32_t depth) {
    uint32_t bufflen = depth + strlen(node->name) + 1;
    char buff[bufflen];
    char *ptr = buff;

    for(uint32_t i = 0; i < depth; i++) {
        *ptr++ = '-';
    }

    strcpy(ptr, node->name);
    buff[bufflen] = '\0';
    log(buff);

    node_t *child;
    LIST_FOR_EACH_ENTRY(child, &node->children, list) {
        traverse_log(child, depth + 1);
    }
}

static INITCALL device_init() {
    logf("device - list start");
    traverse_log(&root, 1);
    logf("device - list end");

    return 0;
}

device_initcall(device_init);
