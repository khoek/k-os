#ifndef KERNEL_COMMON_HASHTABLE_H
#define KERNEL_COMMON_HASHTABLE_H

#include "common/math.h"
#include "common/hash.h"
#include "common/list.h"
#include "common/compiler.h"

typedef chain_head_t hashtable_head_t;
typedef chain_node_t hashtable_node_t;

static inline void hashtable_init_size(chain_head_t *hashtable, uint32_t size) {
    for (uint32_t i = 0; i < ((uint32_t) (1 << size)); i++) {
        chain_init(&hashtable[i]);
    }
}

#define hashtable_init(hashtable)                                               \
    hashtable_init_size(hashtable, ARRAY_SIZE(hashtable))

#define hashtable_add(key, node, hashtable)                                     \
    chain_add_head(node, &hashtable[hash(key, HASHTABLE_BITS(hashtable))])

#define hashtable_add_size(key, node, hashtable, size)                          \
    chain_add_head(node, &hashtable[hash(key, size)])

#define hashtable_rm(node)                                                      \
    chain_rm(node)

#define DEFINE_HASHTABLE(name, bits)                                            \
    chain_head_t name[1 << ((bits) - 1)] =                                      \
        { [0 ... ((1 << ((bits) - 1)) - 1)] = CHAIN_HEAD }

#define HASHTABLE_BITS(name)                                                    \
    log2(ARRAY_SIZE(name))

#define HASHTABLE_FOR_EACH_COLLISION(key, pos, hashtable, member)               \
    CHAIN_FOR_EACH_ENTRY(pos, &((hashtable)[hash(key, HASHTABLE_BITS(hashtable))]), member)

#define HASHTABLE_SIZE_FOR_EACH_COLLISION(key, pos, hashtable, size, member)    \
    CHAIN_FOR_EACH_ENTRY(pos, &((hashtable)[hash(key, size)]), member)

#endif
