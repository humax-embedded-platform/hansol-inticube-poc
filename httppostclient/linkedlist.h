#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>
#include <pthread.h>

typedef struct node_t {
    void* data;
    size_t size;
    struct node_t* next;
    struct node_t* prev;
} node_t;

typedef struct linklist_t {
    node_t* head;
    node_t* tail;
    size_t size;
    pthread_mutex_t m;
} linklist_t;

void linklist_init(linklist_t* list);
void linklist_deinit(linklist_t* list);
void linklist_add(linklist_t* list, node_t* item);
int  linklist_isempty(linklist_t* list);
node_t* linklist_find_and_clone(linklist_t* list, int (*condition_cmp)(const void*, const void*), const void* inputcondition, int from_head);
int linklist_find_and_update(linklist_t* list, int (*condition_check_and_modify)(void*, void*), void* inputcondition);
void linklist_remove(linklist_t* list, int (*condition_cmp)(const void*, const void*), const void* inputcondition, int from_head);
void link_list_node_init(node_t* node, void* data, size_t size);
void link_list_node_deinit(node_t* node);
size_t linklist_get_size(linklist_t* list);
void linklist_for_each(linklist_t* list, void (*callback)(void*, size_t));
#endif  // LINKEDLIST_H
