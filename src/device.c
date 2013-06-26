#include <stddef.h>

#include "string.h"
#include "list.h"
#include "init.h"
#include "device.h"
#include "log.h"

static node_t root = {
    .name = "/",
    .parent = NULL,
    .children = LIST_MAKE_HEAD(root.children)
};

static inline void node_init(node_t *node) {
    list_init(&node->children);
}

static inline void node_add(node_t *child, node_t *parent) {
    list_add(&child->list, &parent->children);
    child->parent = parent;
}

void register_bus(bus_t *bus, char *name) {
    bus->node.name = name;
    node_init(&bus->node);
    node_add(&bus->node, &root);

    list_init(&bus->drivers);
    list_init(&bus->devices);
}

void register_driver(driver_t *driver) {
    list_add(&driver->list, &driver->bus->drivers);
}

void register_device(device_t *device, node_t *parent) {
    node_init(&device->node);
    node_add(&device->node, parent);

    list_add(&device->list, &device->bus->devices);
    
    device->driver = device->bus->match(device);

    if(device->driver) {
        device->node.name = device->driver->name(device);
        device->driver->enable(device);
    } else {
        //TODO handle unidentified devices
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

static void traverse_enable(node_t *node) {
    device_t *device = containerof(node, device_t, node);
    device->driver = device->bus->match(device);

    if(device->driver) {
        device->node.name = device->driver->name(device);
        device->driver->enable(device);
    } else {
        //TODO handle unidentified devices
    }

    node_t *child;
    LIST_FOR_EACH_ENTRY(child, &node->children, list) {
        traverse_enable(child);
    }
}

static INITCALL device_init() {
    node_t *bus;
    LIST_FOR_EACH_ENTRY(bus, &root.children, list) {
        node_t *device;
        LIST_FOR_EACH_ENTRY(device, &bus->children, list) {
            traverse_enable(device);
        }
    }

    logf("device - list:");
    traverse_log(&root, 1);

    return 0;
}

device_initcall(device_init);
