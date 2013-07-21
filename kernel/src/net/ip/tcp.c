#include "lib/int.h"
#include "common/swap.h"
#include "mm/cache.h"
#include "net/packet.h"
#include "net/ip/ip.h"
#include "net/ip/tcp.h"
#include "video/log.h"

void tcp_recv(packet_t *packet, void *raw, uint16_t len) {
    tcp_header_t *tcp = packet->tran.buff = raw;
    raw = tcp + 1;
    len -= sizeof(tcp_header_t);

    tcp++; //Silence gcc
}

static uint16_t tcp_bind_port() {
    //TODO actually pick a port, return the port in network byte order
    return 25564;
}

static void tcp_unbind_port(uint16_t port) {
    //TODO free a bound port, parameter is in network byte order
}

typedef struct tcp_data {
    uint16_t local_port;
} tcp_data_t;

static void tcp_open(sock_t *sock) {
    sock->peer.family = AF_UNSPEC;
    sock->peer.addr = (void *) &IP_AND_PORT_NONE;

    tcp_data_t *data = sock->private = kmalloc(sizeof(tcp_data_t));
    data->local_port = tcp_bind_port();
}

static void tcp_close(sock_t *sock) {
    if(sock->peer.family != AF_UNSPEC) {
        //TODO send FINs, ACKs, etc. and block until closed or connection reset
    }

    tcp_unbind_port(((tcp_data_t *) sock->private)->local_port);
    kfree(sock->private, sizeof(tcp_data_t));
}

static bool tcp_connect(sock_t *sock, sock_addr_t *addr) {
    if(addr->family == AF_UNSPEC) {
        sock->peer.family = AF_UNSPEC;
        sock->peer.addr = (void *) &IP_AND_PORT_NONE;
    } else if(addr->family == AF_INET) {
        sock->peer.family = AF_INET;
        sock->peer.addr = addr->addr;
        
        //TODO actually attempt to connect and block until done
    } else {
        return false;
    }

    return true;
}

static uint32_t tcp_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    if(sock->peer.family != AF_INET) {
        //FIXME errno = ENOTCONN
        return -1;
    }

    //TODO add the data in buff to the TCP processing queue

    //TODO put task to sleep

    return len;
}

sock_protocol_t tcp_protocol = {
    .type  = SOCK_STREAM,

    .open  = tcp_open,
    .connect = tcp_connect,
    .send = tcp_send,
    .close = tcp_close,
};
