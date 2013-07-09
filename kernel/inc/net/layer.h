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
void layer_net_ip(net_packet_t *packet, uint8_t protocol, ip_t src_ip, ip_t dst_ip);

//Transport layer
void layer_tran_udp(net_packet_t *packet, ip_t src_ip, ip_t dst_ip, uint16_t src_port, uint16_t dst_port);

//////////  Packet reception  //////////

//Link layer
void recv_link_eth(net_interface_t *interface, void *packet, uint16_t len);

//Network layer
void recv_net_ip(void *packet, uint16_t len);

//Transport layer
void recv_tran_udp(void *packet, uint16_t len);

#endif
