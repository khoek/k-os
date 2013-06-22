#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

#include <stdbool.h>

#include "common.h"

typedef struct list_head list_head_t;

struct list_head {
    list_head_t *prev, *next;
};

#define list_entry(ptr, type, member) containerof(ptr, type, member)
#define list_first(ptr, type, member) list_entry((ptr)->next, type, member)

static inline void list_init(list_head_t *list) {
	list->next = list;
	list->prev = list;
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

static inline void list_rm(list_head_t *old) {
    list_join(old->prev, old->next);
}

static inline bool list_empty(const list_head_t *head) {
	return head->next == head;
}

#define LIST_MAKE_HEAD(name) {&(name), &(name)}
#define LIST_HEAD(name) list_head_t name = LIST_MAKE_HEAD(name)

#define LIST_FOR_EACH(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define LIST_FOR_EACH_ENTRY(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#endif
