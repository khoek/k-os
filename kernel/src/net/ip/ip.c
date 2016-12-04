#include "common/types.h"
#include "lib/string.h"
#include "common/swap.h"
#include "common/list.h"
#include "init/initcall.h"
#include "mm/mm.h"
#include "net/packet.h"
#include "net/interface.h"
#include "net/socket.h"
#include "net/eth/eth.h"
#include "net/ip/arp.h"
#include "net/ip/ip.h"
#include "net/ip/icmp.h"
#include "net/ip/tcp.h"
#include "net/ip/udp.h"
#include "net/ip/dhcp.h"
#include "log/log.h"

#include "checksum.h"

const ip_t IP_BROADCAST = { .addr = {0xFF, 0xFF, 0xFF, 0xFF} };
const ip_t IP_NONE = { .addr = {0x00, 0x00, 0x00, 0x00} };

const ip_and_port_t IP_AND_PORT_NONE = { .port = 0, .ip = { .addr = {0x00, 0x00, 0x00, 0x00} } };

void ip_build(packet_t *packet, uint8_t protocol, ip_t dst) {
    ip_header_t *hdr = kmalloc(sizeof(ip_header_t));

    hdr->version_ihl = (IP_V4 << 4) | ((uint8_t) (sizeof(ip_header_t) / sizeof(uint32_t)));
    hdr->dscp_ecn = 0;
    hdr->total_length = swap_uint16(sizeof(ip_header_t) + packet->tran.size + packet->payload.size);
    hdr->ident = 0;
    hdr->flags_frag_off = IP_FLAG_DONT_FRAG;
    hdr->ttl = 0x40;
    hdr->protocol = protocol;
    hdr->src = ((ip_interface_t *) packet->interface->ip_data)->ip_addr;
    hdr->dst = dst;
    hdr->checksum = 0;

    uint32_t sum = 0;
    for (uint32_t len = 0; len < (sizeof(ip_header_t) / sizeof(uint16_t)); len++) {
        sum += ((uint16_t *) hdr)[len];
    }

    hdr->checksum = sum_to_checksum(sum);

    ip_t *dst_copy = kmalloc(sizeof(ip_t));
    memcpy(dst_copy, &dst, sizeof(ip_t));

    packet->state = PSTATE_UNRESOLVED;

    packet->route.protocol = ETH_TYPE_IP;
    packet->route.src.family = AF_INET;
    packet->route.src.addr = ((ip_interface_t *) packet->interface->ip_data)->ip_addr.addr;
    packet->route.dst.family = AF_INET;
    packet->route.dst.addr = dst_copy;

    packet->net.buff = hdr;
    packet->net.size = sizeof(ip_header_t);
}

void ip_handle(packet_t *packet, void *raw, uint16_t len) {
    ip_header_t *ip = packet->net.buff = raw;
    raw += sizeof(ip_header_t);
    len -= sizeof(ip_header_t);

    if(IP_VERSION(ip->version_ihl) != 0x04) {
        kprintf("ip - unsupported version number (0x%02X)", IP_VERSION(ip->version_ihl));
    } else {
        arp_cache_store(packet->interface, &eth_hdr(packet)->src, &ip->src);

        switch(ip->protocol) {
            case IP_PROT_ICMP: {
                icmp_handle(packet, raw, len);
                break;
            }
            case IP_PROT_TCP: {
                tcp_handle(packet, raw, len);
                break;
            }
            case IP_PROT_UDP: {
                udp_handle(packet, raw, len);
                break;
            }
            default: {
                kprintf("ip - unrecognised protocol (0x%02X)", ip->protocol);
                break;
            }
        }
    }
}

static void inet_callback(listener_t *listener, net_state_t state, net_interface_t *interface) {
    switch(state) {
        case IF_DOWN: {
            if(interface->ip_data) kfree(interface->ip_data);
            interface->ip_data = kmalloc(sizeof(net_interface_t));
            memset(interface->ip_data, 0, sizeof(net_interface_t));
            break;
        }
        default: break;
    };
}

static listener_t inet_listener = {
    .callback = (callback_t) inet_callback,
};

static INITCALL inet_init() {
    register_net_state_listener(&inet_listener);

    return 0;
}

core_initcall(inet_init);
