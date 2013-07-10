#ifndef KERNEL_NET_LAYER_H
#define KERNEL_NET_LAYER_H

#include "lib/int.h"

#include "net/types.h"

//////////  Address resolution  //////////

void arp_resolve(net_interface_t *interface, packet_t *packet, ip_t ip);

//////////  Packet construction  //////////

//Generic
packet_t * packet_alloc(void *payload, uint16_t len);
void packet_send(net_interface_t *interface, packet_t *packet);

//Link layer
void eth_build(packet_t *packet, uint16_t type, mac_t src, mac_t dst);

//Network layer
void arp_build(packet_t *packet, uint16_t op, mac_t sender_mac, mac_t target_mac, ip_t sender_ip, ip_t target_ip);
void ip_build(packet_t *packet, uint8_t protocol, mac_t src_mac, mac_t dst_mac, ip_t src_ip, ip_t dst_ip);

//Transport layer
void icmp_build(packet_t *packet, uint8_t type, uint8_t code, uint32_t other, mac_t src_mac, mac_t dst_mac, ip_t src_ip, ip_t dst_ip);
void udp_build(packet_t *packet, mac_t src, mac_t dst, ip_t src_ip, ip_t dst_ip, uint16_t src_port, uint16_t dst_port);

//////////  Packet reception  //////////

//Link layer
void eth_recv(net_interface_t *interface, void *packet, uint16_t len);

//Network layer
void arp_recv(net_interface_t *interface, packet_t *packet, void *raw, uint16_t len);
void ip_recv(net_interface_t *interface, packet_t *packet, void *raw, uint16_t len);

//Transport layer
void icmp_recv(net_interface_t *interface, packet_t *packet, void *raw, uint16_t len);
void tcp_recv(net_interface_t *interface, packet_t *packet, void *raw, uint16_t len);
void udp_recv(net_interface_t *interface, packet_t *packet, void *raw, uint16_t len);

#endif
