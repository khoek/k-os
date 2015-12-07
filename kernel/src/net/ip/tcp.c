#include "lib/int.h"
#include "lib/string.h"
#include "lib/rand.h"
#include "common/list.h"
#include "common/swap.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"
#include "mm/mm.h"
#include "mm/cache.h"
#include "time/timer.h"
#include "net/socket.h"
#include "net/packet.h"
#include "net/ip/af_inet.h"
#include "net/ip/ip.h"
#include "net/ip/tcp.h"

#include "checksum.h"

#define TCP_SYN_RETRYS 5
#define TCP_TIMEOUT    500

#define TCP_LISTEN_HASHTABLE_BITS 6

typedef enum tcp_state {
    TCP_RETRY,
    TCP_SYN_SENT,
    TCP_SYN_RECIEVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT,
    TCP_CLOSED,
} tcp_state_t;

typedef struct tcp_data_listen {
    net_interface_t *interface;

    spinlock_t lock;

    uint32_t backlog;
    semaphore_t semaphore;

    list_head_t queue;
    DECLARE_HASHTABLE(connections, TCP_LISTEN_HASHTABLE_BITS);
} tcp_data_listen_t;

typedef struct tcp_data_conn {
    net_interface_t *interface;

    spinlock_t lock;

    tcp_state_t state;

    uint32_t next_local_ack;
    uint32_t next_local_seq;

    uint32_t next_peer_seq;

    uint8_t *recv_buff;
    uint32_t recv_buff_front;
    uint32_t recv_buff_back;
    uint32_t recv_buff_size;

    bool recv_waiting;
    uint8_t retrys;

    semaphore_t semaphore;

    list_head_t queue;
} tcp_data_conn_t;

typedef struct tcp_pending {
    list_head_t list;

    ip_t dst_ip;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window_size;
    uint16_t urgent;
    uint16_t src_port_net;
    uint16_t dst_port_net;

    packet_callback_t callback;

    sock_buff_t payload;
} tcp_pending_t;

#define FREELIST_END    (~((uint32_t) 0))

#define PORT_MAX        65535
#define PORT_NUM        (PORT_MAX + 1)

#define EPHEMERAL_START 49152
#define EPHEMERAL_END   65535
#define EPHEMERAL_NUM   ((EPHEMERAL_END - EPHEMERAL_START) + 1)

static sock_t * ports_sock[PORT_NUM];
static uint32_t ephemeral_ports_next[EPHEMERAL_NUM];
static uint32_t ephemeral_ports_prev[EPHEMERAL_NUM];
static uint32_t ephemeral_next = 0;
static DEFINE_SPINLOCK(port_lock);

static uint16_t tcp_bind_port(sock_t *sock) {
    uint32_t flags;
    spin_lock_irqsave(&port_lock, &flags);

    uint32_t port = 0; //FIXME handle returns of 0 (out of ports) from this function
    if(ephemeral_next != FREELIST_END) {
        port = ephemeral_next + EPHEMERAL_START;

        ports_sock[ephemeral_next + EPHEMERAL_START] = sock;
        ephemeral_next = ephemeral_ports_next[ephemeral_next];
    }

    spin_unlock_irqstore(&port_lock, flags);

    return swap_uint16(port);
}

static void tcp_unbind_port(uint16_t port) {
    port = swap_uint16(port);

    uint32_t flags;
    spin_lock_irqsave(&port_lock, &flags);

    ports_sock[port] = NULL;
    ephemeral_ports_prev[ephemeral_next] = port - EPHEMERAL_START;
    ephemeral_ports_next[port - EPHEMERAL_START] = ephemeral_next;
    ephemeral_next = port - EPHEMERAL_START;

    spin_unlock_irqstore(&port_lock, flags);
}

static void tcp_build(packet_t *packet, ip_t dst_ip, uint32_t seq_num, uint32_t ack_num, uint16_t flags, uint16_t window_size, uint16_t urgent, uint16_t src_port_net, uint16_t dst_port_net) {
    tcp_header_t *hdr = kmalloc(sizeof(tcp_header_t));

    hdr->src_port = src_port_net;
    hdr->dst_port = dst_port_net;
    hdr->seq_num = seq_num;
    hdr->ack_num = ack_num;
    hdr->data_off_flags = ((5 & 0x000F) << 4) | (flags);
    hdr->window_size = swap_uint16(window_size);
    hdr->urgent = swap_uint16(urgent);

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

static void tcp_callback(packet_t *packet, sock_t *sock) {
    tcp_data_conn_t *data = sock->private;

    uint32_t flags;
    spin_lock_irqsave(&data->lock, &flags);

    if(data->state == TCP_SYN_SENT && packet->result != PRESULT_SUCCESS) {
        data->state = TCP_CLOSED;

        semaphore_up(&data->semaphore);
    }

    spin_unlock_irqstore(&data->lock, flags);
}

//should only be called when ((tcp_data_conn_t *) sock->private)->lock is held (or when the sock is being constructed and is therefore not visible)
static void tcp_reset(sock_t *sock) {
    tcp_data_conn_t *data = sock->private;

    data->next_local_seq = data->next_local_ack = rand32();
    data->next_peer_seq = 0;
    data->recv_buff_front = 0;
    data->recv_buff_back = 0 - 1;
    data->recv_buff_size = PAGE_SIZE;
    data->recv_waiting = false;
}

//should only be called when ((tcp_data_conn_t *) sock->private)->lock is NOT held
static void tcp_queue_send(sock_t *sock, tcp_pending_t *pending) {
    tcp_data_conn_t *data = sock->private;

    packet_t *packet = packet_create(data->interface, (packet_callback_t) tcp_callback, sock, pending->payload.buff, pending->payload.size);
    tcp_build(packet, pending->dst_ip, pending->seq_num, pending->ack_num, pending->flags, pending->window_size, pending->urgent, pending->src_port_net, pending->dst_port_net);

    //unlock to prevent deadlocks where tcp_callback is called further down in the call stack
    spin_unlock(&data->lock);
    packet_send(packet);
    spin_lock(&data->lock);

    //TODO set resend timer (only if !(sock->flags & SOCK_FLAG_SHUT_WR))
}

//should only be called when ((tcp_data_conn_t *) sock->private)->lock is held
static void tcp_queue_add(sock_t *sock, uint16_t flags, uint16_t window_size, uint16_t urgent, void *payload_buff, uint32_t payload_size) {
    tcp_data_conn_t *data = sock->private;
    ip_and_port_t *peer_addr = sock->peer.addr;

    tcp_pending_t *pending = kmalloc(sizeof(tcp_pending_t));

    pending->dst_ip = peer_addr->ip;
    pending->seq_num = swap_uint32(data->next_local_seq);
    pending->ack_num = swap_uint32(data->next_peer_seq);
    pending->flags = flags;
    pending->window_size = window_size;
    pending->urgent = urgent;
    pending->src_port_net = ((ip_and_port_t *) sock->local.addr)->port;
    pending->dst_port_net = peer_addr->port;
    pending->payload.buff = payload_buff;
    pending->payload.size = payload_size;

    data->next_local_seq += payload_size;

    list_add_before(&pending->list, &data->queue);

    tcp_queue_send(sock, pending);
}

static void tcp_control_send(sock_t *sock, uint16_t flags, uint16_t window_size) {
    tcp_data_conn_t *data = sock->private;
    ip_and_port_t *peer_addr = sock->peer.addr;

    packet_t *packet = packet_create(data->interface, NULL, NULL, NULL, 0);
    tcp_build(packet, peer_addr->ip, swap_uint32(data->next_local_ack), swap_uint32(data->next_peer_seq), flags, swap_uint16(window_size), 0, ((ip_and_port_t *) sock->local.addr)->port, peer_addr->port);
    packet_send(packet);
}

static void tcp_resend_syn(sock_t *sock) {
    tcp_data_conn_t *data = sock->private;

    if(sock->flags & SOCK_FLAG_CLOSED || data->state == TCP_CLOSED) return;

    uint32_t flags;
    spin_lock_irqsave(&data->lock, &flags);

    data->state = TCP_SYN_SENT;
    tcp_queue_add(sock, TCP_FLAG_SYN, 14600, 0, NULL, 0);

    spin_unlock_irqstore(&data->lock, flags);
}

static void tcp_sock_handle(sock_t *sock, tcp_header_t *tcp, void *raw, uint16_t len) {
    tcp_data_conn_t *data = sock->private;

    uint32_t ack_num = swap_uint32(tcp->ack_num);
    uint32_t seq_num = swap_uint32(tcp->seq_num);

    switch(data->state) {
        case TCP_SYN_SENT: {
            if(tcp->data_off_flags & TCP_FLAG_ACK && tcp->data_off_flags & TCP_FLAG_SYN) {
                if(ack_num > data->next_local_ack) {
                    for(uint32_t i = 0; i < ack_num - data->next_local_ack; i++) {
                        list_rm(&list_first(&data->queue, tcp_pending_t, list)->list);
                    }

                    data->next_local_ack = ack_num;
                }

                data->state = TCP_ESTABLISHED;
                data->next_peer_seq = seq_num + len + 1;

                sock->flags |= SOCK_FLAG_CONNECTED;

                tcp_control_send(sock, TCP_FLAG_ACK, tcp->window_size);

                semaphore_up(&data->semaphore);
            } else {
                data->retrys++;

                if(data->retrys > TCP_SYN_RETRYS) {
                    data->state = TCP_CLOSED;
                    semaphore_up(&data->semaphore);

                    break;
                }

                if(tcp->data_off_flags & TCP_FLAG_ACK) {
                    data->next_local_ack = ack_num;
                } else {
                    data->next_local_ack = 0;
                }

                tcp_control_send(sock, TCP_FLAG_RST | TCP_FLAG_ACK, tcp->window_size);

                tcp_reset(sock);

                data->state = TCP_RETRY;
                timer_create(TCP_TIMEOUT, (timer_callback_t) tcp_resend_syn, sock);
            }

            break;
        }
        case TCP_SYN_RECIEVED: {
            if(data->next_peer_seq == seq_num) {
                if(tcp->data_off_flags & TCP_FLAG_ACK && !(tcp->data_off_flags & TCP_FLAG_SYN)) {
                    if(ack_num != data->next_local_ack) {
                        break;
                    }

                    data->state = TCP_ESTABLISHED;
                    data->next_local_ack++;
                    data->next_local_seq++;

                    sock->flags |= SOCK_FLAG_CONNECTED;
                }
            }

            break;
        }
        case TCP_ESTABLISHED: {
            if(tcp->data_off_flags & TCP_FLAG_ACK) {
                if(ack_num > data->next_local_ack) {
                    for(uint32_t i = 0; i < ack_num - data->next_local_ack; i++) {
                        list_rm(&list_first(&data->queue, tcp_pending_t, list)->list);
                    }

                    data->next_local_ack = ack_num;
                }
            }

            if(len > 0) {
                if(data->next_peer_seq > seq_num) {
                    return;
                } else if (data->next_peer_seq < seq_num) {
                    tcp_control_send(sock, TCP_FLAG_ACK, tcp->window_size);
                    return;
                } else {
                    data->next_peer_seq = seq_num + len;

                    if(!(sock->flags & SOCK_FLAG_SHUT_RD)) {
                        if(data->recv_buff_front != data->recv_buff_back - 1) {
                            for(uint32_t i = 0; i < len; i++) {
                                if(data->recv_buff_front == data->recv_buff_back - 1) {
                                    break;
                                }

                                data->recv_buff[data->recv_buff_front] = ((uint8_t *) raw)[i];
                                data->recv_buff_front++;
                            }
                        }
                    }

                    if(data->recv_waiting) {
                        semaphore_up(&data->semaphore);
                    }
                }
            }

            if(tcp->data_off_flags & TCP_FLAG_FIN) {
                sock->flags |= SOCK_FLAG_SHUT_RD;

                data->next_peer_seq = seq_num + 1;
            }

            if(tcp->data_off_flags & TCP_FLAG_RST) {
                data->state = TCP_CLOSED;
            }

            if(len > 0 || tcp->data_off_flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)) {
                tcp_control_send(sock, TCP_FLAG_ACK, tcp->window_size);
            }

            break;
        }
        default: break;
    }
}
void tcp_handle(packet_t *packet, void *raw, uint16_t len) {
    tcp_header_t *tcp = packet->tran.buff = raw;
    raw = tcp + 1;
    len = swap_uint16(ip_hdr(packet)->total_length) - ((IP_IHL(ip_hdr(packet)->version_ihl) * sizeof(uint32_t)) + (TCP_DATA_OFF(tcp->data_off_flags) * sizeof(uint32_t)));

    uint32_t flags;
    spin_lock_irqsave(&port_lock, &flags);

    sock_t *sock = ports_sock[swap_uint16(tcp->dst_port)];

    spin_unlock_irqstore(&port_lock, flags);

    if(sock) {
        if(sock->flags & SOCK_FLAG_LISTENING) {
            tcp_data_listen_t *data = sock->private;

            spin_lock_irqsave(&data->lock, &flags);

            sock_t *child;
            HASHTABLE_FOR_EACH_COLLISION((*((uint32_t *) &ip_hdr(packet)->src)) * (*((uint16_t *) &tcp->src_port)), child, data->connections, node) {
                if(!memcmp(&((ip_and_port_t *) child->peer.addr)->ip, &ip_hdr(packet)->src, sizeof(ip_t))
                    && ((ip_and_port_t *) child->peer.addr)->port == tcp->src_port
                    && ((ip_and_port_t *) child->local.addr)->port == tcp->dst_port) {
                    tcp_sock_handle(child, tcp, raw, len);

                    break;
                }
            }

            if(tcp->data_off_flags & TCP_FLAG_SYN && !(tcp->data_off_flags & TCP_FLAG_ACK) && data->backlog) {
                data->backlog--;

                child = sock_open(&af_inet, &tcp_protocol);

                ip_and_port_t *peer = kmalloc(sizeof(ip_and_port_t));
                peer->ip = ip_hdr(packet)->src;
                peer->port = tcp->src_port;

                child->peer.family = AF_INET;
                child->peer.addr = peer;
                child->local = sock->local;
                child->flags |= SOCK_FLAG_CONNECTED | SOCK_FLAG_CHILD;

                tcp_data_conn_t *child_data = child->private = kmalloc(sizeof(tcp_data_conn_t));

                tcp_reset(child);

                child_data->retrys = 0;
                child_data->recv_buff = page_to_virt(alloc_page(0));

                list_init(&child_data->queue);
                spinlock_init(&child_data->lock);
                semaphore_init(&child_data->semaphore, 0);

                child_data->state = TCP_SYN_RECIEVED;
                child_data->interface = net_primary_interface();

                child_data->next_local_ack = rand32();
                child_data->next_local_seq = child_data->next_local_ack;
                child_data->next_peer_seq = swap_uint32(tcp->seq_num) + len + 1;

                uint32_t flags3;
                spin_lock_irqsave(&child_data->lock, &flags3);

                list_add_before(&child->list, &data->queue);
                hashtable_add((*((uint32_t *) &ip_hdr(packet)->src)) * (*((uint16_t *) &tcp->src_port)), &child->node, data->connections);

                tcp_queue_add(child, TCP_FLAG_SYN | TCP_FLAG_ACK, 14600, 0, NULL, 0);
                child_data->next_local_ack++;

                semaphore_up(&data->semaphore);

                spin_unlock_irqstore(&child_data->lock, flags3);
            }

            spin_unlock_irqstore(&data->lock, flags);
        } else {
            tcp_data_conn_t *data = sock->private;

            spin_lock_irqsave(&data->lock, &flags);

            if(!memcmp(&((ip_and_port_t *) sock->peer.addr)->ip, &ip_hdr(packet)->src, sizeof(ip_t))
                && ((ip_and_port_t *) sock->peer.addr)->port == tcp->src_port
                && ((ip_and_port_t *) sock->local.addr)->port == tcp->dst_port) {
                tcp_sock_handle(sock, tcp, raw, len);
            }

            spin_unlock_irqstore(&data->lock, flags);
        }
    }
}

static void tcp_open(sock_t *sock) {
    sock->peer.family = AF_UNSPEC;
    sock->peer.addr = (void *) &IP_AND_PORT_NONE;

    sock->private = NULL;
}

static void tcp_close(sock_t *sock) {
    if(sock->flags & SOCK_FLAG_LISTENING) {
        tcp_data_listen_t *data = sock->private;

        uint32_t flags;
        spin_lock_irqsave(&data->lock, &flags);

        sock_t *child;
        LIST_FOR_EACH_ENTRY(child, &data->queue, list) {
            sock_close(child);
        }

        list_init(&data->queue);

        tcp_unbind_port(((ip_and_port_t *) sock->local.addr)->port);

        spin_unlock_irqstore(&data->lock, flags);
    } else {
        tcp_data_conn_t *data = sock->private;

        uint32_t flags;
        spin_lock_irqsave(&data->lock, &flags);

        if(data->state != TCP_CLOSED) {
            if(sock->peer.family == AF_INET) {
                tcp_control_send(sock, TCP_FLAG_RST | TCP_FLAG_ACK, 0);
            }
        }

        if(sock->flags & SOCK_FLAG_CHILD) {
            list_rm(&sock->list);
            hashtable_rm(&sock->node);
        } else if(sock->local.addr) {
            tcp_unbind_port(((ip_and_port_t *) sock->local.addr)->port);
        }

        //Cycle the lock, to ensure that no-one who aquired a ref to the socket uses it after sock->private is freed
        spin_unlock(&data->lock);
        spin_lock(&data->lock);
        spin_unlock_irqstore(&data->lock, flags);

        if(!(sock->flags & SOCK_FLAG_CHILD) && sock->local.addr) {
            kfree(sock->local.addr, sizeof(ip_and_port_t));
        }
    }

    kfree(sock->private, sock->flags & SOCK_FLAG_LISTENING ? sizeof(tcp_data_listen_t) : sizeof(tcp_data_conn_t));
}

static bool tcp_listen(sock_t *sock, uint32_t backlog) {
    tcp_data_listen_t *data = kmalloc(sizeof(tcp_data_listen_t));
    spinlock_init(&data->lock);
    semaphore_init(&data->semaphore, 0);
    data->interface = net_primary_interface();
    data->backlog = backlog && backlog < SOMAXCONN ? backlog : SOMAXCONN;
    list_init(&data->queue);
    hashtable_init(data->connections);

    if(!(sock->flags & SOCK_FLAG_BOUND)) {
        ip_and_port_t *local = kmalloc(sizeof(ip_and_port_t));
        local->ip = IP_ANY;
        local->port = tcp_bind_port(sock);
    }

    sock->flags |= SOCK_FLAG_LISTENING;

    uint32_t flags;
    spin_lock_irqsave(&data->lock, &flags);

    sock->private = data;

    spin_unlock_irqstore(&data->lock, flags);

    return true;
}

static sock_t * tcp_accept(sock_t *sock) {
    sock_t *child = NULL;

    uint32_t flags;
    spin_lock_irqsave(&port_lock, &flags);

    if(sock->flags & SOCK_FLAG_LISTENING) {
        tcp_data_listen_t *data = sock->private;

        if(sock->flags & SOCK_FLAG_SHUT_RDWR) {
            //FIXME errno = EINVAL
        } else {
            spin_unlock_irqstore(&port_lock, flags);
            semaphore_down(&data->semaphore);
            spin_lock_irqsave(&port_lock, &flags);

            if(sock->flags & SOCK_FLAG_SHUT_RDWR) {
                //FIXME errno = EINVAL
            } else {
                uint32_t flags2;
                spin_lock_irqsave(&data->lock, &flags2);

                child = list_first(&data->queue, sock_t, list);
                list_rm(&child->list);

                spin_unlock_irqstore(&data->lock, flags2);
            }
        }
    } else {
        //FIXME errno = EINVAL
    }

    spin_unlock_irqstore(&port_lock, flags);

    return child;
}

static bool tcp_bind(sock_t *sock, sock_addr_t *addr) {
    if(addr->family == AF_INET) {
        if(memcmp(&((ip_and_port_t *) addr->addr)->ip, &IP_ANY, sizeof(ip_t))) {
            //TODO check whether this IP is reachable, if it is, add this socket to a list of sockets bound to this address

            //FIXME errno = EADDRNOTAVAIL

            return false;
        }

        sock->local.family = AF_INET;
        sock->local.addr = addr->addr;

        uint16_t port = swap_uint16(((ip_and_port_t *) sock->local.addr)->port);
        uint16_t ephemeral_port_offset = port - EPHEMERAL_START;

        uint32_t flags;
        spin_lock_irqsave(&port_lock, &flags);

        if(ports_sock[((ip_and_port_t *) sock->local.addr)->port]) {
            spin_unlock_irqstore(&port_lock, flags);

            //FIXME errno = EADDRINUSE
            return false;
        }

        if(port >= EPHEMERAL_START) {
            if(ephemeral_next == ephemeral_port_offset) {
                tcp_bind_port(sock);
            } else {
                ports_sock[port] = sock;

                ephemeral_ports_prev[ephemeral_ports_next[ephemeral_port_offset]] = ephemeral_ports_prev[ephemeral_port_offset];
                ephemeral_ports_next[ephemeral_ports_prev[ephemeral_port_offset]] = ephemeral_ports_next[ephemeral_port_offset];
            }
        } else {
            ports_sock[port] = sock;
        }

        spin_unlock_irqstore(&port_lock, flags);

        sock->flags |= SOCK_FLAG_BOUND;
    }

    return true;
}

static bool tcp_connect(sock_t *sock, sock_addr_t *addr) {
    if(addr->family == AF_INET) {
        sock->peer.family = AF_INET;
        sock->peer.addr = addr->addr;

        tcp_data_conn_t *data = sock->private = kmalloc(sizeof(tcp_data_conn_t));
        tcp_reset(sock);

        data->retrys = 0;
        data->recv_buff = page_to_virt(alloc_page(0));

        list_init(&data->queue);
        spinlock_init(&data->lock);
        semaphore_init(&data->semaphore, 0);

        uint32_t flags;
        spin_lock_irqsave(&data->lock, &flags);

        data->state = TCP_SYN_SENT;
        data->interface = net_primary_interface();

        if(!(sock->flags & SOCK_FLAG_BOUND)) {
            ip_and_port_t *local = kmalloc(sizeof(ip_and_port_t));
            local->ip = IP_ANY;
            local->port = tcp_bind_port(sock);

            sock->local.family = AF_INET;
            sock->local.addr = local;
        }

        tcp_queue_add(sock, TCP_FLAG_SYN, 14600, 0, NULL, 0);
        data->next_local_seq++;

        spin_unlock_irqstore(&data->lock, flags);

        semaphore_down(&data->semaphore);

        spin_lock_irqsave(&data->lock, &flags);

        tcp_state_t state = data->state;

        spin_unlock_irqstore(&data->lock, flags);

        if(state == TCP_ESTABLISHED) {
            return true;
        }
    } else {
        //FIXME errno = EAFNOSUPPORT
    }

    return false;
}

static bool tcp_shutdown(sock_t *sock, int how) {
    if(sock->peer.family == AF_INET) {
        tcp_data_conn_t *data = sock->private;

        uint32_t flags;
        spin_lock_irqsave(&data->lock, &flags);

        if(how == SHUT_RDWR || (sock->flags & SOCK_FLAG_SHUT_WR && how & SHUT_RD) || (sock->flags & SOCK_FLAG_SHUT_RD && how & SHUT_WR)) {
            data->state = TCP_CLOSED;
            tcp_control_send(sock, TCP_FLAG_RST | TCP_FLAG_ACK, 0);
        }

        spin_unlock_irqstore(&data->lock, flags);
    } else {
        return false;
    }

    return true;
}

static uint32_t tcp_send(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    tcp_data_conn_t *data = sock->private;

    uint32_t f;
    spin_lock_irqsave(&data->lock, &f);

    if(data->state == TCP_CLOSED) {
        spin_unlock_irqstore(&data->lock, f);

        //FIXME errno = ECONNRESET
        return -1;
    }

    if(sock->peer.family != AF_INET) {
        spin_unlock_irqstore(&data->lock, f);

        //FIXME errno = ENOTCONN
        return -1;
    }

    if(sock->flags & SOCK_FLAG_SHUT_WR) {
        spin_unlock_irqstore(&data->lock, f);

        //FIXME errno = EPIPE
        return -1;
    }

    //TODO split packets which are too long
    tcp_queue_add(sock, TCP_FLAG_ACK, 14600, 0, buff, len);

    spin_unlock_irqstore(&data->lock, f);

    return len;
}

static uint32_t tcp_recv(sock_t *sock, void *buff, uint32_t len, uint32_t flags) {
    if(sock->peer.family != AF_INET) {
        //FIXME errno = ENOTCONN
        return -1;
    }

    tcp_data_conn_t *data = sock->private;

    uint32_t f;
    spin_lock_irqsave(&data->lock, &f);

    if(data->recv_buff_back + 1 == data->recv_buff_front && data->state == TCP_CLOSED) {
        spin_unlock_irqstore(&data->lock, f);

        //FIXME errno = ECONNRESET
        return -1;
    }

    for(uint32_t i = 0; i < len; i++) {
        if(data->recv_buff_back + 1 == data->recv_buff_front) {
            i--;

            if(sock->flags & SOCK_FLAG_SHUT_RD) {
                len = i;
                goto out;
            }

            data->recv_waiting = true;

            spin_unlock_irqstore(&data->lock, f);

            semaphore_down(&data->semaphore);

            spin_lock_irqsave(&data->lock, &f);

            semaphore_init(&data->semaphore, 0);

            if(sock->flags & SOCK_FLAG_SHUT_RD) {
                len = i;
                goto out;
            }
        } else {
            data->recv_buff_back = (data->recv_buff_back + 1) % data->recv_buff_size;

            ((uint8_t *) buff)[i] = data->recv_buff[data->recv_buff_back];
        }
    }

out:
    spin_unlock_irqstore(&data->lock, f);
    return len;
}

sock_protocol_t tcp_protocol = {
    .type     = SOCK_STREAM,

    .open     = tcp_open,
    .close    = tcp_close,
    .listen   = tcp_listen,
    .accept   = tcp_accept,
    .bind     = tcp_bind,
    .connect  = tcp_connect,
    .shutdown = tcp_shutdown,
    .send     = tcp_send,
    .recv     = tcp_recv,
};

static INITCALL ephemeral_init() {
    for(uint32_t i = 0; i < PORT_NUM; i++) {
        ports_sock[i] = NULL;
    }

    for(uint32_t i = 0; i < EPHEMERAL_NUM; i++) {
        ephemeral_ports_next[i] = i + 1;
        ephemeral_ports_prev[i] = i - 1;
    }
    ephemeral_ports_next[EPHEMERAL_NUM - 1] = FREELIST_END;
    ephemeral_ports_prev[0] = FREELIST_END;

    return 0;
}

core_initcall(ephemeral_init);
