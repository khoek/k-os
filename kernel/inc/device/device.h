#ifndef KERNEL_DEVICE_DEVICE_H
#define KERNEL_DEVICE_DEVICE_H

#include <stdbool.h>

#include "common/list.h"

typedef struct node node_t;
typedef struct bus bus_t;
typedef struct driver driver_t;
typedef struct device device_t;

struct node {
    char *name;
    node_t *parent;
    list_head_t children;

    list_head_t list;
};

struct bus {
    node_t node;
    list_head_t drivers;
    list_head_t devices;
    list_head_t unident_devices;

    bool (*match)(device_t *, driver_t *);

    list_head_t list;
};

struct driver {
    bus_t *bus;

    char * (*name)(device_t *);

    bool (*probe)(device_t *);

    void (*enable)(device_t *);
    void (*disable)(device_t *);
    void (*destroy)(device_t *);

    list_head_t list;
};

struct device {
    node_t node;
    bus_t *bus;
    driver_t *driver;

    void *private;

    list_head_t list;
};

//requires valid device.match
void register_bus(bus_t *bus, char *name);

//requires valid driver.bus, driver.name, driver.probe, driver.enable, driver.disable, driver.destroy
void register_driver(driver_t *driver);

//requires valid device.bus
void register_device(device_t *device, node_t *parent);

#endif
