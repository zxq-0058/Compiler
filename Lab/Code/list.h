#ifndef _LIST_H
#define _LIST_H

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;

#define offsetof(TYPE, MEMBER) ((u64) & ((TYPE *)0)->MEMBER)
#define container_of(ptr, type, field) ((type *)((void *)(ptr) - (u64)(&(((type *)(0))->field))))

#define container_of_safe(ptr, type, field)             \
    ({                                                  \
        typeof(ptr) __ptr = (ptr);                      \
        type *__obj = container_of(__ptr, type, field); \
        (__ptr ? __obj : NULL);                         \
    })

struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

static inline void init_list_head(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

static inline void list_add(struct list_head *new, struct list_head *head) {
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

static inline void list_append(struct list_head *new, struct list_head *head) {
    struct list_head *tail = head->prev;
    return list_add(new, tail);
}

static inline void list_del(struct list_head *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static inline int list_empty(struct list_head *head) { return (head->prev == head && head->next == head); }

#define next_container_of_safe(obj, type, field)                                \
    ({                                                                          \
        typeof(obj) __obj = (obj);                                              \
        (__obj ? container_of_safe(((__obj)->field).next, type, field) : NULL); \
    })

#define list_entry(ptr, type, field) container_of(ptr, type, field)

#define for_each_in_list(elem, type, field, head)                                    \
    for (elem = container_of((head)->next, type, field); &((elem)->field) != (head); \
         elem = container_of(((elem)->field).next, type, field))

#define for_each_in_list_from(elem, type, field, start) \
    for (elem = (start); &((elem)->field) != NULL; elem = container_of(((elem)->field).next, type, field))

#define __for_each_in_list_safe(elem, tmp, type, field, head)                                             \
    for (elem = container_of((head)->next, type, field), tmp = next_container_of_safe(elem, type, field); \
         &((elem)->field) != (head); elem = tmp, tmp = next_container_of_safe(tmp, type, field))

#define for_each_in_list_safe(elem, tmp, field, head) __for_each_in_list_safe(elem, tmp, typeof(*elem), field, head)

#endif