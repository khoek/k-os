#include "lib/int.h"
#include "lib/string.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/protocols.h"
#include "net/interface.h"
#include "net/layer.h"
#include "video/log.h"

packet_t * packet_create(net_interface_t *interface, void *payload, uint16_t len) {
    packet_t *packet = kmalloc(sizeof(packet_t));
    memset(packet, 0, sizeof(packet_t));

    packet->state = P_UNRESOLVED;
    packet->interface = interface;
    packet->payload = payload;
    packet->payload_size = len;

    return packet;
}

static void packet_dispatch(packet_t *packet) {
    packet->interface->tx_send(packet->interface, packet);

    if(packet->link.ptr) kfree(packet->link.ptr, packet->link_size);
    if(packet->net.ptr) kfree(packet->net.ptr, packet->net_size);
    if(packet->tran.ptr) kfree(packet->tran.ptr, packet->tran_size);
    if(packet->payload) kfree(packet->payload, packet->payload_size);

    kfree(packet, sizeof(packet_t));
}

void packet_send(packet_t *packet) {
    if(packet->state == P_UNRESOLVED) {
        packet->interface->hard.resolve(packet);
    } else {
        packet->interface->hard.build_hdr(packet);
        packet_dispatch(packet);
    }
}
