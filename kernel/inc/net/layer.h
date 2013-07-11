#ifndef KERNEL_NET_LAYER_H
#define KERNEL_NET_LAYER_H

#include "lib/int.h"

#include "net/types.h"

//////////  Packet construction  //////////

//Generic
packet_t * packet_create(net_interface_t *interface, void *payload, uint16_t len);
void packet_send(packet_t *packet);

//Network layer
void arp_build(packet_t *packet, uint16_t op, mac_t sender_mac, mac_t target_mac, ip_t sender_ip, ip_t target_ip);
void ip_build(packet_t *packet, uint8_t protocol, ip_t dst_ip);

//Transport layer
void icmp_build(packet_t *packet, uint8_t type, uint8_t code, uint32_t other, ip_t dst_ip);
void udp_build(packet_t *packet, ip_t dst_ip, uint16_t src_port, uint16_t dst_port);

//////////  Packet reception  //////////

//Network layer
void arp_recv(packet_t *packet, void *raw, uint16_t len);
void ip_recv(packet_t *packet, void *raw, uint16_t len);

//Transport layer
void icmp_recv(packet_t *packet, void *raw, uint16_t len);
void tcp_recv(packet_t *packet, void *raw, uint16_t len);
void udp_recv(packet_t *packet, void *raw, uint16_t len);

#endif
