#include "net/types.h"
#include "net/ip/raw.h"

static void raw_open(sock_t *sock) {
}

static void raw_close(sock_t *sock) {
}

sock_protocol_t raw_protocol = {
    .type  = SOCK_RAW,
        
    .open  = raw_open,
    .close = raw_close,
};
