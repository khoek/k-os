#ifndef KERNEL_NET_LAYER_H
#define KERNEL_NET_LAYER_H

#include "lib/int.h"

#include "net/types.h"

//////////  Packet construction  //////////

//Generic
net_packet_t * packet_alloc(void *payload, uint16_t len);
void packet_free(net_packet_t *packet);
void packet_send(net_interface_t *interface, net_packet_t *packet);

//Link layer
void layer_link_eth(net_packet_t *packet, uint16_t type, mac_t src, mac_t dst);

//Network layer
void layer_net_arp(net_packet_t *packet, uint16_t op, mac_t sender_mac, mac_t target_mac, ip_t sender_ip, ip_t target_ip);
void layer_net_ip(net_packet_t *packet, uint8_t protocol, mac_t src_mac, mac_t dst_mac, ip_t src_ip, ip_t dst_ip);

//Transport layer
void layer_tran_icmp(net_packet_t *packet, uint8_t type, uint8_t code, uint32_t other, mac_t src_mac, mac_t dst_mac, ip_t src_ip, ip_t dst_ip);
void layer_tran_udp(net_packet_t *packet, mac_t src, mac_t dst, ip_t src_ip, ip_t dst_ip, uint16_t src_port, uint16_t dst_port);

//////////  Packet reception  //////////

//Link layer
void recv_link_eth(net_interface_t *interface, void *packet, uint16_t len);

//Network layer
void recv_tran_icmp(net_interface_t *interface, void *packet, uint16_t len);
void recv_net_arp(net_interface_t *interface, void *packet, uint16_t len);
void recv_net_ip(net_interface_t *interface, void *packet, uint16_t len);

//Transport layer
void recv_tran_tcp(net_interface_t *interface, void *packet, uint16_t len);
void recv_tran_udp(net_interface_t *interface, void *packet, uint16_t len);

#endif
