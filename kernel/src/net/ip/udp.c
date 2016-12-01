#include "common/types.h"
#include "common/swap.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "mm/mm.h"
#include "net/packet.h"
#include "net/interface.h"
#include "net/ip/ip.h"
#include "net/ip/udp.h"
#include "net/ip/dhcp.h"
#include "net/ip/nbns.h"
#include "log/log.h"

#include "checksum.h"

#define FREELIST_END    (~((uint32_t) 0))

#define EPHEMERAL_START 49152
#define EPHEMERAL_END   65535
#define EPHEMERAL_NUM   ((EPHEMERAL_END - EPHEMERAL_START) + 1)

static uint32_t ephemeral_ports_list[EPHEMERAL_NUM];
static uint32_t ephemeral_next = EPHEMERAL_START;
static DEFINE_SPINLOCK(tcp_ephemeral_lock);

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

void udp_handle(packet_t *packet, void *raw, uint16_t len) {
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
    uint32_t flags;
    spin_lock_irqsave(&tcp_ephemeral_lock, &flags);

    uint32_t port = 0;
    if(ephemeral_next != FREELIST_END) {
        port = ephemeral_next;
        ephemeral_next = ephemeral_ports_list[ephemeral_next];
    }

    spin_unlock_irqstore(&tcp_ephemeral_lock, flags);

    return swap_uint16(port);
}

static void udp_unbind_port(uint16_t port) {
    port = swap_uint16(port);

    uint32_t flags;
    spin_lock_irqsave(&tcp_ephemeral_lock, &flags);

    ephemeral_ports_list[port] = ephemeral_next;
    ephemeral_next = port;

    spin_unlock_irqstore(&tcp_ephemeral_lock, flags);
}

typedef struct udp_data {
    uint16_t local_port;

    semaphore_t semaphore;
} udp_data_t;

static void udp_open(sock_t *sock) {
    sock->peer.family = AF_UNSPEC;
    sock->peer.addr = (void *) &IP_AND_PORT_NONE;

    udp_data_t *data = sock->private = kmalloc(sizeof(udp_data_t));
    semaphore_init(&data->semaphore, 0);
    data->local_port = udp_bind_port();
}

static void udp_close(sock_t *sock) {
    udp_unbind_port(((udp_data_t *) sock->private)->local_port);
    kfree(sock->private);
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

static void udp_callback(packet_t *packet, sock_t *sock) {
    udp_data_t *data = (udp_data_t *) sock->private;

    semaphore_up(&data->semaphore);
}

static uint32_t udp_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    if(sock->peer.family != AF_INET) {
        //FIXME errno = EDESTADDRREQ
        return -1;
    }

    ip_and_port_t *addr_data = (ip_and_port_t *) sock->peer.addr;
    addr_data = (void *) ((uint32_t) addr_data);

    udp_data_t *data = (udp_data_t *) sock->private;

    packet_t *packet = packet_create(net_primary_interface(), (packet_callback_t) udp_callback, sock, buff, len);
    udp_build(packet, addr_data->ip, data->local_port, addr_data->port);
    packet_send(packet);

    semaphore_down(&data->semaphore);

    return len;
}

sock_protocol_t udp_protocol = {
    .type  = SOCK_DGRAM,

    .open  = udp_open,
    .connect = udp_connect,
    .send = udp_send,
    .close = udp_close,
};
