#ifndef LIBSYS_ARPA_INET_H
#define LIBSYS_ARPA_INET_H

#include <inttypes.h>

static inline uint32_t htonl(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

static inline uint16_t htons(uint16_t val) {
    return (val << 8) | (val >> 8);
}

static inline uint32_t ntohl(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

static inline uint16_t ntohs(uint16_t val) {
    return (val << 8) | (val >> 8);
}

#endif
