#include "lib/int.h"
#include "common/init.h"
#include "mm/cache.h"
#include "net/socket.h"

typedef struct sock_raw {
    uint16_t protocol;
} sock_raw_t;

static int raw_socket(int protocol, sock_t *sock) {
    if(protocol > MAX_UINT16) return -1;

    sock_raw_t *raw = sock->private = kmalloc(sizeof(sock_raw_t));
    raw->protocol = (uint16_t) protocol;

    return 0;
}

static sock_provider_t raw_provider = {
    .socket = raw_socket
};

static sock_provider_t * raw_get_provider(int protocol) {
    return &raw_provider;
}

static sock_type_t raw_type = {
    .family = PF_INET,
    .type = SOCK_RAW,

    .provider = raw_get_provider
};

static INITCALL raw_provider_register() {
    register_sock_type(&raw_type);

    return 0;
}

core_initcall(raw_provider_register);
