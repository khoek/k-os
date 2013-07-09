#include "lib/int.h"
#include "common/swap.h"
#include "net/layer.h"
#include "net/protocols.h"
#include "video/log.h"

void recv_tran_tcp(net_interface_t *interface, void *packet, uint16_t len) {
    tcp_header_t *tcp = (tcp_header_t *) packet;
    packet += sizeof(tcp_header_t);
    len -= sizeof(tcp_header_t);

    logf("tcp - src: %u dst: %u", swap_uint16(tcp->src_port), swap_uint16(tcp->dst_port));
}
