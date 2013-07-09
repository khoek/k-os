#include "lib/int.h"
#include "lib/string.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/protocols.h"
#include "net/interface.h"

packet_t * packet_alloc(void *payload, uint16_t len) {
    packet_t *packet = kmalloc(sizeof(packet_t));
    memset(packet, 0, sizeof(packet_t));

    packet->payload = payload;
    packet->payload_size = len;

    return packet;
}

void packet_free(packet_t *packet) {
    kfree(packet, sizeof(packet_t));
}

void packet_send(net_interface_t *interface, packet_t *packet) {
    interface->tx_send(interface, packet);

    if(packet->link.ptr) kfree(packet->link.ptr, packet->link_size);
    if(packet->net.ptr) kfree(packet->net.ptr, packet->net_size);
    if(packet->tran.ptr) kfree(packet->tran.ptr, packet->tran_size);
}
