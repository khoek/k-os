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

static inline void list_init(list_head_t *entry) {
	entry->next = entry;
	entry->prev = entry;
}

static inline bool list_empty(const list_head_t *head) {
	return head->next == head;
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
	list_head_t *first;

	if (!list_empty(head)) {
		first = head->next;
		list_move_before(first, head);
	}
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
