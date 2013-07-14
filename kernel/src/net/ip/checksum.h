#include "lib/int.h"

static inline uint16_t sum_to_checksum(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }

    return ~sum;
}
