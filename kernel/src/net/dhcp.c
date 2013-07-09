#include "lib/string.h"
#include "lib/rand.h"
#include "common/swap.h"
#include "mm/cache.h"
#include "net/types.h"
#include "net/layer.h"
#include "net/dhcp.h"
#include "video/log.h"

#include "eth.h"
#include "ip.h"

#define END_PADDING 28

#define DHCP_PORT_CLIENT 68
#define DHCP_PORT_SERVER 67

#define OP_REQUEST 1
#define OP_REPLY   2

#define HTYPE_ETH  1

#define MAGIC_COOKIE 0x63825363

#define OPT_PAD               0
#define OPT_SUBNET_MASK       1
#define OPT_ROUTER            3
#define OPT_DNS               6
#define OPT_REQUESTED_IP_ADDR 50
#define OPT_LEASE_TIME        51
#define OPT_MESSAGE_TYPE      53
#define OPT_SERVER_ID         54
#define OPT_PARAMETER_REQUEST 55
#define OPT_END               255

typedef struct dhcp_packet {
    uint8_t  op;        //Op code
    uint8_t  htype;     //Hardware address type
    uint8_t  hlen;      //Hardware address length
    uint8_t  hops;      //Private relay agent data
    uint32_t xid;       //Random transaction id
    uint16_t secs;      //Seconds elapsed
    uint16_t flags;     //Flags
    ip_t     ciaddr;    //Client IP address
    ip_t     yiaddr;    //Your IP address
    ip_t     siaddr;    //Server IP address
    ip_t     giaddr;    //Relay agent IP address
    mac_t    chaddr;    //Client MAC address
    uint8_t  pad1[10];  //Zero
    char     sname[64]; //Server host name
    char     file[128]; //Boot file name
    uint32_t cookie;    //Magic Cookie
} PACKED dhcp_packet_t;

static uint8_t opts_discover[] = {
    OPT_MESSAGE_TYPE,
    1,
    OP_REQUEST,

    OPT_PARAMETER_REQUEST,
    3,
    OPT_SUBNET_MASK,
    OPT_ROUTER,
    OPT_DNS,

    OPT_END
};

void dhcp_send_packet(net_interface_t *interface, uint32_t xid, uint8_t *opts, uint32_t opts_size) {
    dhcp_packet_t *dhcp = kmalloc(sizeof(dhcp_packet_t) + sizeof(opts) + END_PADDING);
    memset(dhcp, 0, sizeof(dhcp_packet_t));

    dhcp->op = OP_REQUEST;
    dhcp->htype = HTYPE_ETH;
    dhcp->hlen = sizeof(mac_t);
    dhcp->xid = xid;
    dhcp->chaddr = interface->mac;
    dhcp->cookie = swap_uint32(MAGIC_COOKIE);

    memcpy(dhcp + 1, opts, sizeof(opts));

    net_packet_t *packet = packet_alloc(dhcp, sizeof(dhcp_packet_t) + sizeof(opts) + END_PADDING);
    layer_tran_udp(packet, interface->mac, MAC_BROADCAST, IP_NONE, IP_BROADCAST, DHCP_PORT_CLIENT, DHCP_PORT_SERVER);
    packet_send(interface, packet);

    kfree(dhcp, sizeof(dhcp_packet_t) + sizeof(opts) + END_PADDING);
}

void dhcp_start(net_interface_t *interface) {
    dhcp_send_packet(interface, rand32(), opts_discover, sizeof(opts_discover));

    logf("dhcp - discover request sent");
}
