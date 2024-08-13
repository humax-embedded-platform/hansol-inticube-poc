#include "linkedlist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void linklist_init(linklist_t* list) {
    if (list == NULL) return;

    list->head = NULL;
    list->tail = NULL; // Initialize tail
    list->size = 0;
    pthread_mutex_init(&list->m, NULL);
}

void linklist_deinit(linklist_t* list) {
    if (list == NULL) return;

    pthread_mutex_lock(&list->m);

    node_t* current = list->head;
    while (current != NULL) {
        node_t* next = current->next;
        link_list_node_deinit(current);
        free(current);
        current = next;
    }

    pthread_mutex_unlock(&list->m);
    pthread_mutex_destroy(&list->m);

    list->head = NULL;
    list->tail = NULL; // Reset tail
    list->size = 0;
}

void linklist_add(linklist_t* list, node_t* item) {
    if (list == NULL || item == NULL) return;

    pthread_mutex_lock(&list->m);

    item->next = list->head;
    item->prev = NULL;

    if (list->head != NULL) {
        list->head->prev = item;
    }

    list->head = item;

    if (list->tail == NULL) {
        list->tail = item;
    }

    list->size++;

    pthread_mutex_unlock(&list->m);
}

void linklist_remove(linklist_t* list, node_t* item) {
    if (list == NULL || item == NULL) return;

    pthread_mutex_lock(&list->m);

    if (item->prev != NULL) {
        item->prev->next = item->next;
    } else {
        list->head = item->next;
    }

    if (item->next != NULL) {
        item->next->prev = item->prev;
    } else {
        list->tail = item->prev;
    }

    link_list_node_deinit(item);
    free(item);

    list->size--;

    pthread_mutex_unlock(&list->m);
}

node_t* linklist_find(linklist_t* list, int (*condition)(const void*, const void*), const void* arg, int find_from_tail) {
    if (list == NULL || condition == NULL) return NULL;

    pthread_mutex_lock(&list->m);

    node_t* current = find_from_tail ? list->tail : list->head;

    while (current != NULL) {
        if (condition(current->data, arg)) {
            pthread_mutex_unlock(&list->m);
            return current;  // Return the node if the condition is met
        }
        current = find_from_tail ? current->prev : current->next;
    }

    pthread_mutex_unlock(&list->m);
    return NULL;  // Return NULL if no matching node is found
}

void link_list_node_init(node_t* node, void* data, size_t size) {
    if (node == NULL || data == NULL) return;

    node->data = malloc(size);
    if (node->data != NULL) {
        memcpy(node->data, data, size);
        node->size = size;
        node->next = NULL;
        node->prev = NULL; // Initialize prev
    }
}

void link_list_node_deinit(node_t* node) {
    if (node == NULL) return;

    if (node->data != NULL) {
        free(node->data);
        node->data = NULL;
    }
    node->size = 0;
    node->next = NULL;
    node->prev = NULL;
}
