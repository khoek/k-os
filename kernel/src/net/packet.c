#include "lib/int.h"
#include "lib/string.h"
#include "sync/semaphore.h"
#include "mm/cache.h"
#include "net/packet.h"
#include "net/interface.h"
#include "log/log.h"

packet_t * packet_create(net_interface_t *interface, packet_callback_t callback, void *data, void *payload, uint16_t len) {
    packet_t *packet = kmalloc(sizeof(packet_t));
    memset(packet, 0, sizeof(packet_t));

    packet->state = PSTATE_UNRESOLVED;
    packet->result = PRESULT_SUCCESS;
    packet->data = data;
    packet->callback = callback;
    packet->interface = interface;
    packet->payload.buff = payload;
    packet->payload.size = len;

    return packet;
}

void packet_destroy(packet_t *packet) {
    if(packet->callback) packet->callback(packet, packet->data);

    if(packet->link.buff) kfree(packet->link.buff, packet->link.size);
    if(packet->net.buff) kfree(packet->net.buff, packet->net.size);
    if(packet->tran.buff) kfree(packet->tran.buff, packet->tran.size);
    if(packet->payload.buff) kfree(packet->payload.buff, packet->payload.size);

    kfree(packet, sizeof(packet_t));
}

void packet_send(packet_t *packet) {
    if(packet->state == PSTATE_UNRESOLVED) {
        packet->interface->link_layer.resolve(packet);
    } else {
        if(packet->result == PRESULT_SUCCESS) {
            packet->interface->link_layer.build_hdr(packet);
            packet->interface->send(packet);
        } else {
            packet_destroy(packet);
        }
    }
}

uint32_t packet_expand(void *buff, packet_t *packet, uint32_t minimum_size) {
    uint16_t len = packet->link.size + packet->net.size + packet->tran.size + packet->payload.size;
    memcpy(buff, packet->link.buff, packet->link.size);
    buff += packet->link.size;
    memcpy(buff, packet->net.buff, packet->net.size);
    buff += packet->net.size;
    memcpy(buff, packet->tran.buff, packet->tran.size);
    buff += packet->tran.size;
    memcpy(buff, packet->payload.buff, packet->payload.size);
    buff += packet->payload.size;

    if(len < minimum_size) {
        memset(buff, 0, 60 - minimum_size);
        len = minimum_size;
    }

    return len;
}
