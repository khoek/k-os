#include "lib/int.h"
#include "common/compiler.h"

#define ICMP_TYPE_ECHO_REQUEST 8
#define ICMP_TYPE_ECHO_REPLY   0

#define ICMP_CODE_ECHO_REQUEST 0
#define ICMP_CODE_ECHO_REPLY   0

typedef struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t other;
} PACKED icmp_header_t;
