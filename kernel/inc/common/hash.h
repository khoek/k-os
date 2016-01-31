#ifndef KERNEL_COMMON_HASH_H
#define KERNEL_COMMON_HASH_H

#include "common/types.h"
#include "lib/string.h"
#include "common/compiler.h"

#define HASH_PRIME 0x9e370001UL

#define hash(val, bits)                                                         \
    (sizeof(val) <= 4 ? hash_32(val, bits) : hash_64(val, bits))

static inline uint64_t hash_64(uint64_t key, uint32_t bits) {
	uint64_t hash = key;

	uint64_t n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;

	return hash >> (64 - bits);
}

static inline uint32_t hash_32(uint32_t key, uint32_t bits) {
	uint32_t hash = key * HASH_PRIME;

	return hash >> (32 - bits);
}

static inline uint32_t str_to_key(const char *str, uint32_t len) {
    uint32_t key = 1;

    for(uint32_t i = 0; i < len; i++) {
        key *= str[i];
    }

    return key;
}

#endif
