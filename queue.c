/**
 * Program: queue.c
 * Author: Vytor Calixto
 */
#include "queue.h"
#include <stdio.h>

void queue_append(queue_t **queue, queue_t *elem) {
    // Elemento inexistente
    if(!elem) return;
    // Elemento pertencente a outra fila
    if((elem->next != NULL) || (elem->prev != NULL)) return;
    // Fila vazia
    if(!(*queue)) {
        elem->next = elem;
        elem->prev = elem;
        *queue = elem;
        return;
    }

    queue_t *aux = *queue;
    // Enquanto o próximo elemento não é o início da fila
    while(aux->next != *queue) {
        aux = aux->next;
    }
    elem->prev = aux;
    elem->next = aux->next;
    aux->next->prev = elem;
    aux->next = elem;

    return;
}

queue_t *queue_remove(queue_t **queue, queue_t *elem) {
    if(!(*queue)) {
        return NULL;
    }
    if(!elem) {
        return NULL;
    }

    // Verifica se está na fila
    int achou = 0;
    queue_t *aux;
    for(aux = *queue; aux->next != *queue; aux = aux->next) {
        if(aux == elem) {
            achou = 1;
        }
    }

    if(!achou && elem->next != *queue) return NULL;

    // Único elemento da fila
    if(elem->next == elem) {
        elem->next = NULL;
        elem->prev = NULL;
        *queue = NULL;
    } else {
        // Se remove o primeiro da fila
        if((*queue) == elem) *queue = elem->next;
        queue_t *prev = elem->prev;
        queue_t *next = elem->next;
        prev->next = next;
        next->prev = prev;
        elem->next = NULL;
        elem->prev = NULL;
    }
    return elem;
}

int queue_size(queue_t *queue) {
    // Fila vazia
    if(!queue) return 0;
    int size = 1;
    queue_t *aux;
    for(aux = queue; aux->next != queue; aux = aux->next) ++size;
    return size;
}

void queue_print(char *name, queue_t *queue, void print_elem(void*)) {
    printf("%s [", name);
    // Fila vazia
    if(!queue) {
        print_elem(queue);
        puts("]");
        return;
    }
    queue_t *aux;
    for(aux = queue; aux->next != queue; aux = aux->next) print_elem(aux);
    print_elem(aux);
    puts("]");
    return;
}
