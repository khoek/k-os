#include <stddef.h>

#include "list.h"
#include "init.h"
#include "device.h"
#include "log.h"

static node_t root = {
    .name = "/",
    .parent = NULL,
    .children = LIST_MAKE_HEAD(root.children)
};

LIST_HEAD(buses);

static inline void node_init(node_t *node) {
    list_init(&node->children);
}

static inline void node_add(node_t *child, node_t *parent) {
    list_add(&child->list, &parent->children);
    child->parent = parent;
}

void register_bus(char *name, bus_t *bus) {
    bus->node.name = name;
    node_init(&bus->node);
    node_add(&bus->node, &root);

    list_init(&bus->drivers);
    list_init(&bus->devices);

    list_add(&bus->list, &buses);
}

void register_driver(driver_t *driver) {
    list_add(&driver->list, &driver->bus->drivers);
}

void register_device(device_t *device) {
    node_init(&device->node);
    node_add(&device->node, &device->bus->node);

    list_add(&device->list, &device->bus->devices);
}

static INITCALL device_init() {
    bus_t *bus;
    LIST_FOR_EACH_ENTRY(bus, &buses, list) {
        device_t *device;
        LIST_FOR_EACH_ENTRY(device, &bus->devices, list) {
            device->driver = bus->match(device);

            if(device->driver) {
                device->node.name = device->driver->name(device);
                device->driver->enable(device);
            } else {
                //TODO handle unidentified devices
            }
        };
    };

    return 0;
}

device_initcall(device_init);
