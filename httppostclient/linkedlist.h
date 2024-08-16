#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>
#include <pthread.h>

typedef struct node_t {
    void* data;
    size_t size;
    struct node_t* next;
    struct node_t* prev; // Pointer to the previous node
} node_t;

typedef struct linklist_t {
    node_t* head;  // Pointer to the first node
    node_t* tail;  // Pointer to the last node
    size_t size;   // Number of elements in the list
    pthread_mutex_t m;  // Mutex for thread safety
} linklist_t;

void linklist_init(linklist_t* list);
void linklist_deinit(linklist_t* list);
void linklist_add(linklist_t* list, node_t* item);
int  linklist_isempty(linklist_t* list);
node_t* linklist_find_and_clone(linklist_t* list, int (*condition_cmp)(const void*, const void*), const void* inputcondition, int from_head);
void linklist_remove(linklist_t* list, int (*condition_cmp)(const void*, const void*), const void* inputcondition, int from_head);
void link_list_node_init(node_t* node, void* data, size_t size);
void link_list_node_deinit(node_t* node);

#endif  // LINKEDLIST_H
