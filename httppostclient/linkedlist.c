#include "linkedlist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void linklist_init(linklist_t* list) {
    if (list == NULL) return;

    list->head = NULL;
    list->size = 0;
    pthread_mutex_init(&list->m, NULL);
}

void linklist_deinit(linklist_t* list) {
    if (list == NULL) return;

    pthread_mutex_lock(&list->m);

    node_t* current = list->head;
    while (current != NULL) {
        node_t* next = current->next;
        link_list_node_deinit(current);  // Deinitialize each node
        current = next;
    }

    pthread_mutex_unlock(&list->m);
    pthread_mutex_destroy(&list->m);

    list->head = NULL;
    list->size = 0;
}

void linklist_add(linklist_t* list, node_t* item) {
    if (list == NULL || item == NULL) return;

    pthread_mutex_lock(&list->m);

    item->next = list->head;
    list->head = item;
    list->size++;

    pthread_mutex_unlock(&list->m);
}

void linklist_remove(linklist_t* list, node_t* item) {
    if (list == NULL || item == NULL) return;

    pthread_mutex_lock(&list->m);

    node_t* current = list->head;
    node_t* prev = NULL;

    while (current != NULL) {
        if (current == item) {
            if (prev == NULL) {
                list->head = current->next;
            } else {
                prev->next = current->next;
            }
            link_list_node_deinit(current);  // Deinitialize the node being removed
            list->size--;
            break;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&list->m);
}

void link_list_node_init(node_t* node, void* data, size_t size) {
    if (node == NULL || data == NULL) return;

    node->data = malloc(size);
    if (node->data != NULL) {
        memcpy(node->data, data, size);
        node->size = size;
        node->next = NULL;
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
}

void linklist_remove_with_condition(linklist_t* list, void* input, int (*condition)(const void*, const void*), int (*remove_callback)(const void*)) {
    if (list == NULL || condition == NULL) return;

    pthread_mutex_lock(&list->m);

    node_t* current = list->head;
    node_t* prev = NULL;
    
    while (current != NULL) {
        node_t* next = current->next;

        if (condition(current->data, input)) {
            if(remove_callback != NULL) {
                remove_callback(current->data);
            }

            if (prev == NULL) {
                list->head = next;
            } else {
                prev->next = next;
            }

            link_list_node_deinit(current);
            free(current);
            list->size--;
        } else {
            prev = current;
        }

        current = next;
    }

    pthread_mutex_unlock(&list->m);
}