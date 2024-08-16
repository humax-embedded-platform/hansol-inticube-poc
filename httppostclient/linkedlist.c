#include "linkedlist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void linklist_init(linklist_t* list) {
    if (list == NULL) return;

    list->head = NULL;
    list->tail = NULL;
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
    list->tail = NULL;
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

node_t* linklist_find_and_clone(linklist_t* list, int (*condition_cmp)(const void*, const void*), const void* inputcondition, int from_head) {
    if (list == NULL || condition_cmp == NULL) return NULL;

    pthread_mutex_lock(&list->m);

    node_t* current = from_head ? list->head : list->tail;

    while (current != NULL) {
        if (condition_cmp(current->data, inputcondition)) {
            node_t* clone = (node_t*)malloc(sizeof(node_t));
            if (clone == NULL) {
                perror("Failed to allocate memory for node clone");
                return NULL;
            }

            clone->data = malloc(current->size);
            memcpy(clone->data, current->data, current->size);
            pthread_mutex_unlock(&list->m);
            return clone; 
        }
        current = from_head ? current->next : current->prev;
    }

    pthread_mutex_unlock(&list->m);
    return NULL; 
}

void linklist_remove(linklist_t* list, int (*condition_cmp)(const void*, const void*), const void* inputcondition, int from_head) {
    if (list == NULL || condition_cmp == NULL) return; // Error handling

    pthread_mutex_lock(&list->m);

    node_t* current = from_head ? list->head : list->tail;

    while (current != NULL) {
        if (condition_cmp(current->data, inputcondition)) {

            if (current->prev) {
                current->prev->next = current->next;
            } else {
                list->head = current->next;
            }

            if (current->next) {
                current->next->prev = current->prev;
            } else {
                list->tail = current->prev;
            }

            list->size--;

            link_list_node_deinit(current);

            pthread_mutex_unlock(&list->m);
            return;
        }
        current = from_head ? current->next : current->prev;
    }

    pthread_mutex_unlock(&list->m);
}

int  linklist_isempty(linklist_t* list) {
    int isempty =0;
    pthread_mutex_lock(&list->m);
    isempty = list->size > 0 ? 0 : 1;
    pthread_mutex_unlock(&list->m);
    return isempty;
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

    free(node);
}
