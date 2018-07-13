#ifndef KERNEL_COMMON_LIST_H
#define KERNEL_COMMON_LIST_H

#include <stdbool.h>

#include "common/types.h"
#include "common/compiler.h"

typedef struct list_head list_head_t;

struct list_head {
    list_head_t *prev, *next;
};

#define list_entry(ptr, type, member) containerof(ptr, type, member)
#define list_first(ptr, type, member) list_entry((ptr)->next, type, member)

#define list_is_last(pos, head, member) (&(pos)->member == (head))

#define list_next_unsafe(pos, head, member) list_entry(pos->member.next, typeof(*pos), member)
#define list_next(pos, head, member) \
  ((pos)->member.next == (head) ? NULL : list_next_unsafe(pos, head, member))


static inline void list_init(list_head_t *entry) {
    entry->next = entry;
    entry->prev = entry;
}

static inline bool list_empty(const list_head_t *head) {
    return head->next == head;
}

static inline bool list_is_singular(const list_head_t *head) {
    return !list_empty(head) && (head->next == head->prev);
}

static inline void list_insert(list_head_t *new, list_head_t *prev, list_head_t *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(list_head_t *new, list_head_t *old) {
    list_insert(new, old, old->next);
}

static inline void list_add_before(list_head_t *new, list_head_t *old) {
    list_insert(new, old->prev, old);
}

static inline void list_join(list_head_t *first, list_head_t *second) {
    first->next = second;
    second->prev = first;
}

static inline void list_replace(list_head_t *old, list_head_t *new) {
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}

static inline void list_rm(list_head_t *entry) {
    list_join(entry->prev, entry->next);
}

static inline void list_move(list_head_t *entry, list_head_t *head) {
    list_rm(entry);
    list_add(entry, head);
}

static inline void list_move_before(list_head_t *entry, list_head_t *head) {
    list_rm(entry);
    list_add_before(entry, head);
}

static inline void list_rotate_left(list_head_t *head) {
    if (!list_empty(head)) {
        list_move_before(head->next, head);
    }
}

#define LIST_HEAD(name) { &(name), &(name) }
#define DEFINE_LIST(name) list_head_t name = LIST_HEAD(name)

#define LIST_FOR_EACH(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)
#define LIST_FOR_EACH_ENTRY(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
        !list_is_last(pos, head, member);                      \
        pos = list_next_unsafe(pos, head, member))

typedef struct chain_node chain_node_t;

struct chain_node {
    chain_node_t *next, **pprev;
};

typedef struct chain_head_t {
    chain_node_t *first;
} chain_head_t;

#define chain_entry(ptr, type, member) containerof(ptr,type,member)
#define chain_entry_safe(ptr, type, member) \
    ({ typeof(ptr) ____ptr = (ptr);                         \
       ____ptr ? chain_entry(____ptr, type, member) : NULL; \
    })

static inline void chain_init(chain_head_t *head) {
    head->first = NULL;
}

static inline void chain_init_node(chain_node_t *h) {
    h->next = NULL;
    h->pprev = NULL;
}

static inline bool chain_unhashed(const chain_node_t *h) {
    return !h->pprev;
}

static inline bool chain_empty(const chain_head_t *h) {
    return !h->first;
}

static inline void chain_join(chain_node_t **pprev, chain_node_t *next) {
    *pprev = next;
    if(next) {
        next->pprev = pprev;
    }
}

static inline void chain_rm(chain_node_t *n) {
    chain_join(n->pprev, n->next);
    n->next = NULL;
    n->pprev = NULL;
}

static inline void chain_add_head(chain_node_t *n, chain_head_t *h) {
    chain_node_t *first = h->first;
    n->next = first;
    if(first) {
        first->pprev = &n->next;
    }
    h->first = n;
    n->pprev = &h->first;
}

static inline void chain_add_before(chain_node_t *n, chain_node_t *next) {
    n->pprev = next->pprev;
    n->next = next;
    next->pprev = &n->next;
    *(n->pprev) = n;
}

static inline void chain_add_after(chain_node_t *n, chain_node_t *next) {
    next->next = n->next;
    n->next = next;
    next->pprev = &n->next;

    if(next->next) {
        next->next->pprev = &next->next;
    }
}

static inline void chain_move_list(chain_head_t *old, chain_head_t *new) {
    new->first = old->first;
    if(new->first) {
        new->first->pprev = &new->first;
    }
    old->first = NULL;
}

#define CHAIN_HEAD { .first = NULL }
#define DEFINE_CHAIN(name) chain_head_t name = {  .first = NULL }

#define CHAIN_FOR_EACH(pos, head) \
    for (pos = (head)->first; pos ; pos = pos->next)
#define CHAIN_FOR_EACH_ENTRY(pos, head, member) \
    for (pos = chain_entry_safe((head)->first, typeof(*(pos)), member);      \
         pos;                                                                \
         pos = chain_entry_safe((pos)->member.next, typeof(*(pos)), member))

#endif
