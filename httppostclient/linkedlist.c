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
    if (list == NULL) {
        return;
    }

    pthread_mutex_lock(&list->m);

    node_t* current = list->head;
    while (current != NULL) {
        node_t* next = current->next;
        linklist_node_deinit(current);
        current = next;
    }

    pthread_mutex_unlock(&list->m);
    pthread_mutex_destroy(&list->m);

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void linklist_add(linklist_t* list, node_t* item) {
    pthread_mutex_lock(&list->m);
    if (list == NULL || item == NULL){
        pthread_mutex_unlock(&list->m);
        return;
    }

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

node_t* linklist_find_and_remove(linklist_t* list, int (*condition_cmp)(void*, void*), void* inputcondition, int from_head) {
    pthread_mutex_lock(&list->m);

    if (list == NULL || condition_cmp == NULL) {
        pthread_mutex_unlock(&list->m);
        return NULL;
    }

    node_t* current = from_head ? list->head : list->tail;

    while (current != NULL) {
        if (condition_cmp(current->data, inputcondition)) {
            // Clone the node
            node_t* clone = (node_t*)malloc(sizeof(node_t));
            if (clone == NULL) {
                pthread_mutex_unlock(&list->m);
                return NULL;
            }

            clone->data = malloc(current->size);
            if (clone->data == NULL) {
                free(clone);
                pthread_mutex_unlock(&list->m);
                return NULL;
            }

            clone->size = current->size;
            clone->next = NULL;
            clone->prev = NULL;
            memcpy(clone->data, current->data, current->size);

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

            free(current->data);
            free(current);

            list->size--;

            pthread_mutex_unlock(&list->m);
            return clone;
        }

        current = from_head ? current->next : current->prev;
    }

    pthread_mutex_unlock(&list->m);
    return NULL;
}


void linklist_remove_with_condition(linklist_t* list, int (*condition_cmp)(void*, void*), void* inputcondition, int from_head) {
    pthread_mutex_lock(&list->m);
    if (list == NULL || condition_cmp == NULL) {
        pthread_mutex_unlock(&list->m);
        return;
    }

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

            linklist_node_deinit(current);

            pthread_mutex_unlock(&list->m);
            return;
        }
        current = from_head ? current->next : current->prev;
    }

    pthread_mutex_unlock(&list->m);
}

int  linklist_isempty(linklist_t* list) {
    pthread_mutex_lock(&list->m);
    if(list == NULL) {
        pthread_mutex_unlock(&list->m);
        return 1;
    }

    int isempty =0;
    isempty = list->size > 0 ? 0 : 1;
    pthread_mutex_unlock(&list->m);
    return isempty;
}

void linklist_node_init(node_t* node, void* data, size_t size) {
    if (node == NULL || data == NULL) return;

    node->data = malloc(size);
    if (node->data != NULL) {
        memcpy(node->data, data, size);
        node->size = size;
        node->next = NULL;
        node->prev = NULL;
    }
}

void linklist_node_deinit(node_t* node) {
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

size_t linklist_get_size(linklist_t* list) {
    size_t size =0;

    pthread_mutex_lock(&list->m);
    size = list->size;
    pthread_mutex_unlock(&list->m);

    return size;
}

int linklist_find_and_update(linklist_t* list, int (*condition_check_and_modify)(void*, void*), void* inputcondition) {
    pthread_mutex_lock(&list->m);
    if (list == NULL || condition_check_and_modify == NULL) {
        pthread_mutex_unlock(&list->m);
        return 0;
    }

    node_t* current = list->head;
    while (current != NULL) {
        if (condition_check_and_modify(current->data, inputcondition) == 1) {
            pthread_mutex_unlock(&list->m);
            return 1;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&list->m);

    return 0;
}

void linklist_for_each(linklist_t* list, void (*callback)(void*, size_t)) {
    pthread_mutex_lock(&list->m);
    if (list == NULL || callback == NULL) {
        pthread_mutex_unlock(&list->m);
        return;
    }

    node_t* current = list->head;
    while (current != NULL) {
        callback(current->data, current->size);
        current = current->next;
    }

    pthread_mutex_unlock(&list->m);
}

node_t* linklist_remove_from_tail(linklist_t* list) {

    pthread_mutex_lock(&list->m);
    if (list == NULL || list->tail == NULL){
        pthread_mutex_unlock(&list->m);
        return NULL;
    }

    node_t* removed_node = list->tail;
    if (list->tail->prev != NULL) {
        list->tail = list->tail->prev;
        list->tail->next = NULL;
    } else {
        list->head = list->tail = NULL;
    }
    list->size--;

    node_t* clone = malloc(sizeof(node_t));
    if (clone != NULL) {
        linklist_node_init(clone, removed_node->data, removed_node->size);
    }

    linklist_node_deinit(removed_node);

    pthread_mutex_unlock(&list->m);

    return clone;
}