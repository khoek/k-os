#include "lib/int.h"
#include "common/compiler.h"

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} PACKED udp_header_t;
