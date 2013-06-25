#ifndef KERNEL_DEVICE_H
#define KERNEL_DEVICE_H

#include "list.h"

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

    driver_t * (*match)(device_t *);

    list_head_t list;
};

struct driver {
    bus_t *bus;

    char * (*name)(device_t *);

    void (*enable)(device_t *);
    void (*disable)(device_t *);
    void (*destroy)(device_t *);

    list_head_t list;
};

struct device {
    node_t node;
    bus_t *bus;
    driver_t *driver;

    list_head_t list;
};

void register_bus(char *name, bus_t *bus);
void register_driver(driver_t *driver);
void register_device(device_t *device);

#endif
