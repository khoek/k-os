#include "lib/int.h"
#include "lib/string.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/interface.h"
#include "net/layer.h"
#include "video/log.h"

packet_t * packet_create(net_interface_t *interface, void *payload, uint16_t len) {
    packet_t *packet = kmalloc(sizeof(packet_t));
    memset(packet, 0, sizeof(packet_t));

    packet->state = P_UNRESOLVED;
    packet->interface = interface;
    packet->payload.buff = payload;
    packet->payload.size = len;

    return packet;
}

static void packet_dispatch(packet_t *packet) {
    packet->interface->tx_send(packet->interface, packet);

    if(packet->link.buff   ) kfree(packet->link.buff   , packet->link.size);
    if(packet->net.buff    ) kfree(packet->net.buff    , packet->net.size);
    if(packet->tran.buff   ) kfree(packet->tran.buff   , packet->tran.size);
    if(packet->payload.buff) kfree(packet->payload.buff, packet->payload.size);

    kfree(packet, sizeof(packet_t));
}

void packet_send(packet_t *packet) {
    if(packet->state == P_UNRESOLVED) {
        packet->interface->link_layer.resolve(packet);
    } else {
        packet->interface->link_layer.build_hdr(packet);
        packet_dispatch(packet);
    }
}
