/**
* Program: ppos_core.c
* Author: Vytor Calixto
*/
#include "ppos_data.h"
#include "ppos.h"
#include <stdio.h>
#include <ucontext.h>

task_t *MainContext;
task_t *currentTask;
int lastId = 0;

void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);
    MainContext = (task_t*) malloc(sizeof(struct task_t));
    // main id is set as 0
    MainContext->tid = lastId;
    // currentTask is set to the main
    currentTask = MainContext;
    #ifdef DEBUG
    printf ("ppos_init: Sistema iniciado\n") ;
    #endif
}

int task_create(task_t *task, void (*start_routine)(void *),  void *arg) {
    char *stack;
    getcontext(&(task->context));
    stack = malloc(STACKSIZE);
    if(stack) {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
        task->tid = ++lastId;
        makecontext(&(task->context), (void*)(start_routine), 1, arg);
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
    printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->tid) ;
    #endif
    task_switch(MainContext);
}

int task_id() {
    return currentTask->tid;
}
