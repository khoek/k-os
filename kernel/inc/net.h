#ifndef KERNEL_NET_H
#define KERNEL_NET_H

#include "int.h"
#include "list.h"

typedef struct mac { uint8_t addr[6]; } mac_t;
typedef struct ip { uint8_t addr[4]; } ip_t;

typedef struct net_packet {
    void *link_hdr;
    uint32_t link_len;
    void *net_hdr;
    uint32_t net_len;
    void *tran_hdr;
    uint32_t tran_len;
    void *payload;
    uint32_t payload_len;
} net_packet_t;

#define MAC_BROADCAST ((mac_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} })

#define IP_BROADCAST ((ip_t) { .addr = {0xFF, 0xFF, 0xFF, 0xFF} })
#define IP_NONE      ((ip_t) { .addr = {0x00, 0x00, 0x00, 0x00} })

#define ETH_TYPE_IP    0x0008
#define ETH_TYPE_ARP   0x0608

#define IP_PROT_TCP 0x06
#define IP_PROT_UDP 0x11

typedef enum net_state {
    IF_DHCP,
    IF_UP,
    IF_ERROR
} net_state_t;

typedef struct net_interface net_interface_t;

struct net_interface {
    list_head_t list;

    mac_t mac;
    ip_t ip;
    uint32_t rx_total, tx_total;
    
    net_state_t state;

    void (*rx_poll)(net_interface_t *);
    int32_t (*tx_send)(net_interface_t *, net_packet_t *);
};

void register_net_interface(net_interface_t *interface);
void unregister_net_interface(net_interface_t *interface);

void net_recv(net_interface_t *interface, void *packet, uint16_t length);
void net_send(net_interface_t *interface, net_packet_t *packet);
net_packet_t * net_packet(void *payload, uint16_t len);

void packet_link_eth(net_packet_t *packet, uint16_t type, mac_t src, mac_t dst);
void packet_net_ip(net_packet_t *packet, uint8_t protocol, ip_t src_ip, ip_t dst_ip);
void packet_tran_udp(net_packet_t *packet, ip_t src, ip_t dst, uint16_t src_port, uint16_t dst_port);

#endif
