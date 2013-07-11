#ifndef KERNEL_HASHTABLE_H
#define KERNEL_HASHTABLE_H

#include <stddef.h>
#include <stdbool.h>

#include "common/math.h"
#include "common/hash.h"
#include "common/compiler.h"

typedef struct hlist_node hlist_node_t;

struct hlist_node {
    hlist_node_t *next, **pprev;
};

typedef struct hlist_head_t {
    hlist_node_t *first;
} hlist_head_t;

#define HLIST_HEAD { .first = NULL }
#define DEFINE_HLIST(name) hlist_head_t name = {  .first = NULL }

static inline void hlist_init(hlist_head_t *head) {
    head->first = NULL;
}

static inline void hlist_init_node(hlist_node_t *h) {
    h->next = NULL;
    h->pprev = NULL;
}

static inline bool hlist_unhashed(const hlist_node_t *h) {
    return !h->pprev;
}

static inline bool hlist_empty(const hlist_head_t *h) {
    return !h->first;
}

static inline void hlist_join(hlist_node_t **pprev, hlist_node_t *next) {
    *pprev = next;
    if(next)
        next->pprev = pprev;
}

static inline void hlist_rm(hlist_node_t *n) {
    hlist_join(n->pprev, n->next);
    n->next = NULL;
    n->pprev = NULL;
}

static inline void hlist_add_head(hlist_node_t *n, hlist_head_t *h) {
    hlist_node_t *first = h->first;
    n->next = first;
    if (first)
        first->pprev = &n->next;
    h->first = n;
    n->pprev = &h->first;
}

static inline void hlist_add_before(hlist_node_t *n, hlist_node_t *next) {
    n->pprev = next->pprev;
    n->next = next;
    next->pprev = &n->next;
    *(n->pprev) = n;
}

static inline void hlist_add_after(hlist_node_t *n, hlist_node_t *next) {
    next->next = n->next;
    n->next = next;
    next->pprev = &n->next;

    if(next->next)
        next->next->pprev = &next->next;
}

static inline void hlist_move_list(hlist_head_t *old, hlist_head_t *new) {
    new->first = old->first;
    if (new->first)
        new->first->pprev = &new->first;
    old->first = NULL;
}

#define hlist_entry(ptr, type, member) containerof(ptr,type,member)

#define hlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? hlist_entry(____ptr, type, member) : NULL; \
	})

#define hlist_for_each(pos, head) \
    for (pos = (head)->first; pos ; pos = pos->next)

#define hlist_for_each_entry(pos, head, member)                                 \
    for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member);         \
         pos;                                                                   \
         pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

static inline void hashtable_init_size(hlist_head_t *hashtable, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        hlist_init(&hashtable[i]);
    }
}

#define hashtable_init(hashtable)                                               \
    hash_init_size(hashtable, ARRAY_SIZE(hashtable))

#define hashtable_add(key, node, hashtable)                                     \
    hlist_add_head(node, &hashtable[hash(key, HASHTABLE_BITS(hashtable))])

#define hashtable_rm(node)                                                      \
    hlist_rm(node)

#define DEFINE_HASHTABLE(name, bits)                                            \
    hlist_head_t name[1 << ((bits) - 1)] =                                      \
        { [0 ... ((1 << ((bits) - 1)) - 1)] = HLIST_HEAD }

#define HASHTABLE_BITS(name)                                                    \
    log2(ARRAY_SIZE(name))

#define HASHTABLE_FOR_EACH_COLLISION(key, pos, hashtable, member)               \
    hlist_for_each_entry(pos, &((hashtable)[hash(key, HASHTABLE_BITS(hashtable))]), member)

#endif
