#include "lib/int.h"
#include "lib/string.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/interface.h"

net_packet_t * packet_alloc(void *payload, uint16_t len) {
    net_packet_t *packet = kmalloc(sizeof(net_packet_t));
    memset(packet, 0, sizeof(net_packet_t));

    packet->payload = payload;
    packet->payload_len = len;

    return packet;
}

void packet_free(net_packet_t *packet) {
    kfree(packet, sizeof(net_packet_t));
}

void packet_send(net_interface_t *interface, net_packet_t *packet) {
    interface->tx_send(interface, packet);

    if(packet->link_hdr) kfree(packet->link_hdr, packet->link_len);
    if(packet->net_hdr) kfree(packet->net_hdr, packet->net_len);
    if(packet->tran_hdr) kfree(packet->tran_hdr, packet->tran_len);
}
