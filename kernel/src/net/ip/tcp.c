#include "lib/int.h"
#include "lib/rand.h"
#include "common/swap.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "mm/cache.h"
#include "net/packet.h"
#include "net/ip/ip.h"
#include "net/ip/tcp.h"
#include "video/log.h"

#include "checksum.h"

typedef enum tcp_state {
    TCP_INITIALIZED,
    TCP_SYN_SENT,
    TCP_ESTABLISHED,
    TCP_CLOSED,
} tcp_state_t;

#define FREELIST_END    (~((uint32_t) 0))

#define EPHEMERAL_START 49152
#define EPHEMERAL_END   65535
#define EPHEMERAL_NUM   ((EPHEMERAL_END - EPHEMERAL_START) + 1)

static sock_t * ephemeral_ports_sock[EPHEMERAL_NUM];
static uint32_t ephemeral_ports_list[EPHEMERAL_NUM];
static uint32_t ephemeral_next = EPHEMERAL_START;
static DEFINE_SPINLOCK(tcp_ephemeral_lock);

static void tcp_build(packet_t *packet, ip_t dst_ip, uint32_t seq_num, uint32_t ack_num, uint16_t flags, uint16_t window_size, uint16_t urgent, uint16_t src_port_net, uint16_t dst_port_net) {
    tcp_header_t *hdr = kmalloc(sizeof(tcp_header_t));

    hdr->src_port = src_port_net;
    hdr->dst_port = dst_port_net;
    hdr->seq_num = seq_num;
    hdr->ack_num = ack_num;
    hdr->data_off_flags = ((5 & 0x000F) << 4) | (flags);
    hdr->window_size = swap_uint16(window_size);
    hdr->urgent = urgent;

    uint32_t sum = 0;
    sum += ((uint16_t *) &((ip_interface_t *) packet->interface->ip_data)->ip_addr.addr)[0];
    sum += ((uint16_t *) &((ip_interface_t *) packet->interface->ip_data)->ip_addr.addr)[1];
    sum += ((uint16_t *) &dst_ip)[0];
    sum += ((uint16_t *) &dst_ip)[1];
    sum += swap_uint16((uint16_t) IP_PROT_TCP);
    sum += swap_uint16(sizeof(tcp_header_t) + packet->payload.size);

    sum += hdr->src_port;
    sum += hdr->dst_port;
    sum += ((uint16_t *) &hdr->seq_num)[0];
    sum += ((uint16_t *) &hdr->seq_num)[1];
    sum += ((uint16_t *) &hdr->ack_num)[0];
    sum += ((uint16_t *) &hdr->ack_num)[1];
    sum += hdr->data_off_flags;
    sum += hdr->window_size;
    sum += hdr->urgent;

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
    packet->tran.size = sizeof(tcp_header_t);

    ip_build(packet, IP_PROT_TCP, dst_ip);
}

void tcp_recv(packet_t *packet, void *raw, uint16_t len) {
    tcp_header_t *tcp = packet->tran.buff = raw;
    raw = tcp + 1;
    len -= sizeof(tcp_header_t);

    tcp++; //Silence gcc
}

static uint16_t tcp_bind_port(sock_t *sock) {
    uint32_t flags;
    spin_lock_irqsave(&tcp_ephemeral_lock, &flags);

    uint32_t port = 0; //FIXME handle returns of 0 (out of ports) from this function
    if(ephemeral_next != FREELIST_END) {
        port = ephemeral_next;
        ephemeral_ports_sock[port] = sock;
        ephemeral_next = ephemeral_ports_list[ephemeral_next];
    }

    spin_unlock_irqstore(&tcp_ephemeral_lock, flags);

    return swap_uint16(port);
}

static void tcp_unbind_port(uint16_t port) {
    port = swap_uint16(port);

    uint32_t flags;
    spin_lock_irqsave(&tcp_ephemeral_lock, &flags);

    ephemeral_ports_list[port] = ephemeral_next;
    ephemeral_next = port;

    spin_unlock_irqstore(&tcp_ephemeral_lock, flags);
}

typedef struct tcp_data {
    uint16_t local_port;

    tcp_state_t state;
    semaphore_t semaphore;
} tcp_data_t;

static void tcp_open(sock_t *sock) {
    sock->peer.family = AF_UNSPEC;
    sock->peer.addr = (void *) &IP_AND_PORT_NONE;

    tcp_data_t *data = sock->private = kmalloc(sizeof(tcp_data_t));
    data->state = TCP_INITIALIZED;
    semaphore_init(&data->semaphore, 0);

    //do this last, it publishes the socket
    data->local_port = tcp_bind_port(sock);
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

        tcp_data_t *data = sock->private;

        data->state = TCP_SYN_SENT;

        packet_t *packet = packet_create(net_primary_interface(), NULL, NULL, 0);
        tcp_build(packet, ((ip_and_port_t *) addr->addr)->ip, rand32(), 0, TCP_FLAG_SYN, 14600, 0, ((tcp_data_t *) sock->private)->local_port, ((ip_and_port_t *) addr->addr)->port);
        packet_send(packet);

        semaphore_down(&data->semaphore);

        //TODO unlock the semaphore when syn-ack recieved
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

static INITCALL tcp_ephemeral_init() {
    for(uint32_t i = 0; i < EPHEMERAL_NUM - 1; i++) {
        ephemeral_ports_list[i] = i + 1;
    }
    ephemeral_ports_list[EPHEMERAL_NUM - 1] = FREELIST_END;

    return 0;
}

pure_initcall(tcp_ephemeral_init);
