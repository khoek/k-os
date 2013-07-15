#include "lib/int.h"
#include "common/swap.h"
#include "net/packet.h"
#include "net/ip/tcp.h"
#include "video/log.h"

void tcp_recv(packet_t *packet, void *raw, uint16_t len) {
    tcp_header_t *tcp = packet->tran.buff = raw;
    raw = tcp + 1;
    len -= sizeof(tcp_header_t);

    tcp++; //Silence gcc
}
