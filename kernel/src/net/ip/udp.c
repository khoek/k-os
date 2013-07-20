#include "lib/int.h"
#include "common/swap.h"
#include "sync/atomic.h"
#include "mm/cache.h"
#include "net/packet.h"
#include "net/interface.h"
#include "net/ip/ip.h"
#include "net/ip/udp.h"
#include "net/ip/dhcp.h"
#include "net/ip/nbns.h"
#include "video/log.h"

#include "checksum.h"

void udp_build(packet_t *packet, ip_t dst_ip, uint16_t src_port_net, uint16_t dst_port_net) {
    udp_header_t *hdr = kmalloc(sizeof(udp_header_t));

    hdr->src_port = src_port_net;
    hdr->dst_port = dst_port_net;
    hdr->length = swap_uint16(sizeof(udp_header_t) + packet->payload.size);

    uint32_t sum = 0;
    sum += ((uint16_t *) &((ip_interface_t *) packet->interface->ip_data)->ip_addr.addr)[0];
    sum += ((uint16_t *) &((ip_interface_t *) packet->interface->ip_data)->ip_addr.addr)[1];
    sum += ((uint16_t *) &dst_ip)[0];
    sum += ((uint16_t *) &dst_ip)[1];
    sum += swap_uint16((uint16_t) IP_PROT_UDP);
    sum += swap_uint16(sizeof(udp_header_t) + packet->payload.size);

    sum += hdr->src_port;
    sum += hdr->dst_port;
    sum += hdr->length;

    uint32_t len = packet->payload.size / sizeof(uint8_t);
    uint16_t *ptr = (uint16_t *) packet->payload.buff;
    for (; len > 1; len -= 2) {
        sum += *ptr++;
    }

    if(len) {
        sum += *((uint8_t *) ptr);
    }

    hdr->checksum = sum_to_checksum(sum);

    packet->tran.buff = hdr;
    packet->tran.size = sizeof(udp_header_t);

    ip_build(packet, IP_PROT_UDP, dst_ip);
}

void udp_recv(packet_t *packet, void *raw, uint16_t len) {
    udp_header_t *udp = packet->tran.buff = raw;
    raw += sizeof(udp_header_t);
    len -= sizeof(udp_header_t);

    udp->src_port = swap_uint16(udp->src_port);
    udp->dst_port = swap_uint16(udp->dst_port);

    if(packet->interface->state == IF_UP && udp->src_port == DHCP_PORT_SERVER && udp->dst_port == DHCP_PORT_CLIENT) {
        dhcp_handle(packet, raw, len);
    } else if(packet->interface->state == IF_READY && udp->dst_port == NBNS_PORT) {
        nbns_handle(packet, raw, len);
    }
}

static uint16_t udp_bind_port() {
    //TODO actually pick a port, return the port in network byte order
    return 25564;
}

static void udp_unbind_port(uint16_t port) {
    //TODO free a bound port, parameter is in network byte order
}

typedef struct udp_data {
    uint16_t local_port;
} udp_data_t;

static void udp_open(sock_t *sock) {
    sock->peer.family = AF_UNSPEC;
    sock->peer.addr = (void *) &IP_AND_PORT_NONE;

    udp_data_t *data = sock->private = kmalloc(sizeof(udp_data_t));
    data->local_port = udp_bind_port();
}

static void udp_close(sock_t *sock) {
    udp_unbind_port(((udp_data_t *) sock->private)->local_port);
    kfree(sock->private, sizeof(udp_data_t));
}

static bool udp_connect(sock_t *sock, sock_addr_t *addr) {
    if(addr->family == AF_UNSPEC) {
        sock->peer.family = AF_UNSPEC;
        sock->peer.addr = (void *) &IP_AND_PORT_NONE;
    } else if(addr->family == AF_INET) {
        sock->peer.family = AF_INET;
        sock->peer.addr = addr->addr;
    } else {
        return false;
    }

    return true;
}

static uint32_t udp_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    if(sock->peer.family != AF_INET) {
        //FIXME errno = EDESTADDRREQ
        return -1;
    }

    ip_and_port_t *addr_data = (ip_and_port_t *) sock->peer.addr;
    addr_data = (void *) ((uint32_t) addr_data);

    packet_t *packet = packet_create(net_primary_interface(), buff, len);
    udp_build(packet, addr_data->ip, ((udp_data_t *) sock->private)->local_port, addr_data->port);
    packet_send(packet);

    //TODO put task to sleep

    return len;
}

sock_protocol_t udp_protocol = {
    .type  = SOCK_DGRAM,

    .open  = udp_open,
    .connect = udp_connect,
    .send = udp_send,
    .close = udp_close,
};
