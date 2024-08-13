#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>
#include <pthread.h>

typedef struct node_t {
    void* data;
    size_t size;
    struct node_t* next;
} node_t;

typedef struct linklist_t {
    node_t* head;
    size_t size;
    pthread_mutex_t m;
} linklist_t;

// Function declarations
void linklist_init(linklist_t* list);
void linklist_deinit(linklist_t* list);
void linklist_add(linklist_t* list, node_t* item);
void linklist_remove_with_condition(linklist_t* list, const void* input, int (*condition)(const void*, const void*), int (*remove_callback)(const void*));
void link_list_node_init(node_t* node, void* data, size_t size);
void link_list_node_deinit(node_t*);

#endif  // LINKEDLIST_H
