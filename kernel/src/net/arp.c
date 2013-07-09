#include "lib/int.h"
#include "net/layer.h"
#include "video/log.h"

void recv_net_arp(void *packet, uint16_t len) {
    logf("arp - packet, length %u bytes", len);
}
