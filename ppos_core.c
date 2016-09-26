/**
* Program: ppos_core.c
* Author: Vytor Calixto
*/
#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

int lastId = 0;
task_t MainContext;
task_t dispatcher;
task_t *currentTask;
task_t *readyQueue;

static task_t *scheduler() {
    task_t *aux, *task;
    task = readyQueue;
    aux = readyQueue->next;
    while(aux != readyQueue) {
        #ifdef DEBUG
        printf("id:%d tp:%d - ta:%d\tid:%d ap:%d - aa:%d\n", task->tid, task->priority, task->aging, aux->tid, aux->priority, aux->aging);
        #endif
        if((aux->priority + aux->aging) < (task->priority + task->aging)) {
            --(task->aging);
            task = aux;
        } else {
            --(aux->aging);
        }
        aux = aux->next;
    }
    #ifdef DEBUG
    printf("scheduler: escolhida tarefa %d com prioridade %d (p: %d + a: %d)\n",
        task->tid, task->priority + task->aging, task->priority, task->aging);
    #endif
    task->aging = 0;
    queue_remove((queue_t **) &readyQueue, (queue_t *) task);
    return task;
}

void dispatcher_body () {
    task_t *next;

    while (queue_size((queue_t *)readyQueue) > 0) {
        #ifdef DEBUG
        printf("dispatcher: ready queue size %d\n", queue_size((queue_t *)readyQueue));
        #endif
        next = scheduler() ;  // scheduler é uma função
        if (next) {
            // ações antes de lançar a tarefa "next", se houverem
            next->status = RUNNING;
            task_switch (next) ; // transfere controle para a tarefa "next"
            // ações após retornar da tarefa "next", se houverem
            if(next->status == RUNNING) {
                next->status = READY;
                queue_append((queue_t **) &readyQueue, (queue_t *) next);
            } else if (next->status == FINISHED) {
                    free(next->context.uc_stack.ss_sp);
            }
        }
    }
    task_exit(0);
}

void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);
    // main id is set as 0
    MainContext.tid = lastId;
    // currentTask is set to the main
    currentTask = &MainContext;

    // Dispatcher
    #ifdef DEBUG
    printf("ppos_init: Inicializando dispatcher\n") ;
    #endif
    task_create(&dispatcher, dispatcher_body, "dispatcher");

    // Ready queue
    readyQueue = NULL;

    #ifdef DEBUG
    printf("ppos_init: Dispatcher inicializado\n");
    printf("ppos_init: Sistema iniciado\n") ;
    #endif
}

int task_create(task_t *task, void (*start_routine)(void *),  void *arg) {
    char *stack;
    getcontext(&(task->context));
    stack = malloc(STACKSIZE);
    if(stack) {
        task->next = NULL;
        task->prev = NULL;
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
        task->tid = ++lastId;
        task->status = READY;
        task->priority = 0;
        task->aging = 0;
        makecontext(&(task->context), (void*)(start_routine), 1, arg);
        // Add task to ready queue
        queue_append((queue_t **) &readyQueue, (queue_t *) task);
        #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->tid) ;
        #endif
        return task->tid;
    }
    #ifdef DEBUG
    printf ("task_create: um erro ocorreu") ;
    #endif
    return -1;
}

int task_switch(task_t *task) {
    #ifdef DEBUG
    printf ("task_switch: trocando contexto %d -> %d\n", currentTask->tid, task->tid) ;
    #endif

    task_t *oldTask = currentTask;
    currentTask = task;
    swapcontext(&(oldTask->context), &(currentTask->context));
    return 0;
}

void task_exit(int exit_code) {
    #ifdef DEBUG
    printf("task_exit: tarefa %d sendo encerrada\n", currentTask->tid);
    #endif
    currentTask->status = FINISHED;
    (currentTask->tid!=1) ? task_switch(&dispatcher) : task_switch(&MainContext);
}

int task_id() {
    return currentTask->tid;
}

void task_suspend (task_t *task, task_t **queue) {
    if(!task) {
        task = currentTask;
    }
    task->status = SUSPENDED;
    // remove task from current queue
    queue_remove((queue_t **) &readyQueue, (queue_t *) task);
    queue_append((queue_t **) queue, (queue_t *) task);
}

void task_resume (task_t *task) {
    task->status = READY;
    // inserts in ready queue
    queue_append((queue_t **) &readyQueue, (queue_t *) task);
}

void task_yield() {
    #ifdef DEBUG
    printf("task_yield: tarefa %d liberou o processador\n", currentTask->tid);
    #endif
    task_switch(&dispatcher);
}

void task_setprio (task_t *task, int prio) {
    if(prio < -20 || prio > 20) {
        return;
    }

    if(!task) {
        task = currentTask;
    }
    #ifdef DEBUG
    printf("task_setprio: tarefa %d ganhou prioridade %d\n", task->tid, prio);
    #endif
    task->priority = prio;
}

int task_getprio (task_t *task) {
    return (task) ? task->priority : currentTask->priority;
}
