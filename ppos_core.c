/**
* Program: ppos_core.c
* Author: Vytor Calixto
*/
#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"
#include "hw_disk.h"
#include "ppos_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

int LastId = 0, Ticks = 0, preemption = 1;
task_t MainContext;
task_t dispatcher;
task_t *currentTask;
task_t *readyQueue, *sleepQueue;

// estrutura que define um tratador de sinal
struct sigaction action;

// estrutura de inicialização to timer
struct itimerval timer;

void timerHandler(int signum) {
    if (signum != 14) {
        return;
    }
    ++Ticks;
    if(currentTask->type == USER_TASK && preemption == 1) {
        if(--(currentTask->ticks) <= 0) {
            task_switch(&dispatcher);
        }
    }
}

static task_t *scheduler() {
    if(queue_size((queue_t *)readyQueue) == 0) {
        return NULL;
    }
    task_t *aux, *task;
    task = readyQueue;
    aux = readyQueue->next;
    while(aux != readyQueue) {
        #ifdef DEBUG
        // printf("id:%d tp:%d - ta:%d\tid:%d ap:%d - aa:%d\n", task->tid, task->priority, task->aging, aux->tid, aux->priority, aux->aging);
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
    task->ticks = SYSTEM_TICKS;
    task->aging = 0;
    return (task_t *) queue_remove((queue_t **) &readyQueue, (queue_t *) task);
}

void dispatcher_body () {
    task_t *next;

    while (queue_size((queue_t *)readyQueue) > 0 || queue_size((queue_t *)sleepQueue) > 0) {
        unsigned int tDispatcher = systime();
        if(queue_size((queue_t *) sleepQueue) > 0) {
            task_t *task = sleepQueue, *aux = task->next;
            for(task = sleepQueue; task->next != sleepQueue; task=aux) {
                aux = task->next;
                if(task->wakeup_time <= Ticks) {
                    #ifdef DEBUG
                    printf("timerHandler: acordando tarefa %d (%d) em %d\n", task->tid, task->wakeup_time, Ticks);
                    #endif
                    task_resume((task_t *) queue_remove((queue_t **) &sleepQueue, (queue_t *) task));
                }
            }
            if(sleepQueue->next == sleepQueue) {
                if(task->wakeup_time <= Ticks) {
                    #ifdef DEBUG
                    printf("timerHandler: acordando tarefa %d (%d) em %d\n", task->tid, task->wakeup_time, Ticks);
                    #endif
                    task_resume((task_t *) queue_remove((queue_t **) &sleepQueue, (queue_t *) task));
                }
            }
        }
        next = scheduler() ;  // scheduler é uma função
        dispatcher.proc_time += (systime() - tDispatcher);
        if (next) {
            tDispatcher = systime();
            // ações antes de lançar a tarefa "next", se houverem
            next->status = RUNNING;
            unsigned int t1 = systime();
            next->activations++;
            dispatcher.proc_time += (systime() - tDispatcher);

            #ifdef DEBUG
            printf("dispatcher: indo para a tarefa %d\n", next->tid);
            #endif
            task_switch (next) ; // transfere controle para a tarefa "next"

            // ações após retornar da tarefa "next", se houverem
            tDispatcher = systime();
            next->proc_time += (systime() - t1);
            dispatcher.activations++;
            if(next->status == RUNNING) {
                next->status = READY;
                queue_append((queue_t **) &readyQueue, (queue_t *) next);
            } else if (next->status == FINISHED) {
                free(next->context.uc_stack.ss_sp);
            }
            dispatcher.proc_time += (systime() - tDispatcher);
        }
    }
    task_exit(0);
}

void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);
    readyQueue = NULL;
    // main id is set as 0
    MainContext.tid = LastId;
    MainContext.type = USER_TASK;
    // Add main to readyQueue
    queue_append((queue_t **) &readyQueue, (queue_t *) &MainContext);
    currentTask = &MainContext;

    // Dispatcher
    #ifdef DEBUG
    printf("ppos_init: Inicializando dispatcher\n") ;
    #endif
    task_create(&dispatcher, dispatcher_body, "dispatcher");
    dispatcher.type = SYSTEM_TASK;
    #ifdef DEBUG
    printf("ppos_init: Dispatcher inicializado\n");
    #endif

    action.sa_handler = timerHandler;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

    #ifdef DEBUG
    printf("ppos_init: Timer iniciado\n");
    #endif

    #ifdef DEBUG
    printf("ppos_init: Sistema iniciado\n") ;
    #endif
    task_switch(&dispatcher);
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
        task->tid = ++LastId;
        task->status = READY;
        task->priority = 0;
        task->aging = 0;
        task->type = USER_TASK;
        task->ticks = 0;
        task->exe_init_time = systime();
        task->proc_time = task->activations = 0;
        task->wakeup_time = 0;
        makecontext(&(task->context), (void*)(start_routine), 1, arg);
        // Add task to ready queue if it's not the dispatcher
        if(task->tid != 1) {
            queue_append((queue_t **) &readyQueue, (queue_t *) task);
        }
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
    currentTask->returnValue = exit_code;
    #ifdef DEBUG
    printf("task_exit: tarefa %d sendo encerrada com status %d\n", currentTask->tid, exit_code);
    #endif
    currentTask->status = FINISHED;
    currentTask->exe_end_time = systime();

    // Join queue
    task_t *aux = currentTask->joinQueue;
    while(aux != NULL) {
        task_resume((task_t *) queue_remove((queue_t **) &currentTask->joinQueue, (queue_t *) aux));
        aux = currentTask->joinQueue;
    }

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
    currentTask->tid, currentTask->exe_end_time - currentTask->exe_init_time,
    currentTask->proc_time, currentTask->activations);
    (currentTask->tid!=1) ? task_switch(&dispatcher) : task_switch(&MainContext);
}

int task_id() {
    return currentTask->tid;
}

void task_suspend (task_t *task, task_t **queue) {
    if(!task) {
        task = currentTask;
    }
    #ifdef DEBUG
    printf("task_suspend: suspendendo tarefa %d\n", task->tid);
    #endif
    task->status = SUSPENDED;
    // remove task from current queue
    // queue_remove((queue_t **) &readyQueue, (queue_t *) task);
    queue_append((queue_t **) queue, (queue_t *) task);
}

void task_resume (task_t *task) {
    // inserts in ready queue
    #ifdef DEBUG
    printf("task_resume: resumindo tarefa %d\n", task->tid);
    #endif
    task->status = READY;
    queue_append((queue_t **) &readyQueue, (queue_t *) task);
}

void task_yield() {
    #ifdef DEBUG
    printf("task_yield: tarefa %d liberou o processador\n", currentTask->tid);
    #endif
    task_switch(&dispatcher);
}

void task_setprio(task_t *task, int prio) {
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

unsigned int systime() {
    return Ticks;
}

int task_join (task_t *task) {
    if(!task) {
        return -1;
    }
    if(task->status == FINISHED) {
        return task->returnValue;
    }
    task_suspend(currentTask, &task->joinQueue);
    task_switch(&dispatcher);
    return task->returnValue;
}

void task_sleep (int t) {
    if(t == 0) return;
    currentTask->status = SUSPENDED;
    currentTask->wakeup_time = t*1000 + Ticks;
    queue_append((queue_t **) &sleepQueue, (queue_t *) currentTask);
    #ifdef DEBUG
    printf("task_sleep: tarefa %d dorme até %d\n", currentTask->tid, currentTask->wakeup_time);
    #endif
    task_switch(&dispatcher);
}

int sem_create (semaphore_t *s, int value) {
    if(s == NULL) return -1;
    if(s->live == 1) return -1; // Semáforo existente
    s->lock = 0;
    s->value = value;
    s->queue = NULL;
    s->live = 1;
    s->returnValue = 0;
    return 0;
}

int sem_down (semaphore_t *s) {
    if(s == NULL) return -1;
    if(s->live != 1) return -1;
    preemption = 0;
    #ifdef DEBUG
    printf("sem_down: tarefa %d fazendo down\n", currentTask->tid);
    #endif
    --s->value;
    if(s->value < 0) {
        #ifdef DEBUG
        printf("sem_down: tarefa %d espera semáforo\n", currentTask->tid);
        #endif
        task_suspend(currentTask, &(s->queue));
        preemption = 1;
        task_switch(&dispatcher);
    }
    preemption = 1;
    return (s->returnValue == -1) ? -1 : 0;
}

int sem_up (semaphore_t *s) {
    if(s == NULL) return -1;
    if(s->live != 1) return -1;
    preemption = 0;
    #ifdef DEBUG
    printf("sem_up: tarefa %d fazendo up\n", currentTask->tid);
    #endif
    ++s->value;
    if(s->value <= 0) {
        queue_t *aux = (queue_t *) s->queue;
        task_t *t = (task_t *) queue_remove((queue_t **) &(s->queue), aux);
        #ifdef DEBUG
        printf("sem_up: tarefa %d volta a fila de prontas\n", t->tid);
        #endif
        task_resume(t);
    }
    preemption = 1;
    return 0;
}

int sem_destroy (semaphore_t *s) {
    if(s == NULL) return -1;
    if(s->live != 1) return -1;
    s->live = 0;
    s->returnValue = -1;
    int i;
    for(i=queue_size((queue_t *)s->queue); i>0; --i) {
        queue_t *aux = (queue_t *) s->queue;
        task_t *t = (task_t *) queue_remove((queue_t **) &(s->queue), aux);
        task_resume(t);
    }
    return 0;
}

int mqueue_create (mqueue_t *queue, int max_msgs, int msg_size) {
    if(max_msgs <= 0) return -1;
    if(msg_size <= 0) return -1;
    if(queue == NULL) return -1;
    queue->size = 0;
    queue->max_msgs = max_msgs;
    queue->msg_size = msg_size;
    queue->msgs = malloc(sizeof(void *)*max_msgs*msg_size);
    queue->head = queue->tail = 0;
    sem_create(&(queue->empty_spaces), max_msgs);
    sem_create(&(queue->buffer), 1);
    sem_create(&(queue->msg), 0);
    return 0;
}

int mqueue_send (mqueue_t *queue, void *msg) {
    if(queue == NULL) return -1;
    if(queue->size == -1) return -1;
    sem_down(&(queue->empty_spaces));
    sem_down(&(queue->buffer));
    ++queue->size;
    memcpy(&(queue->msgs[queue->tail]), msg, queue->msg_size);
    queue->tail = (queue->tail + 1) % queue->max_msgs;
    sem_up(&(queue->buffer));
    sem_up(&(queue->msg));
    return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg) {
    if(queue == NULL) return -1;
    if(queue->size == -1) return -1;
    sem_down(&(queue->msg));
    sem_down(&(queue->buffer));
    --queue->size;
    memcpy(msg, &(queue->msgs[queue->head]), queue->msg_size);
    queue->head = (queue->head + 1) % queue->max_msgs;
    sem_up(&(queue->buffer));
    sem_up(&(queue->empty_spaces));
}

int mqueue_destroy (mqueue_t *queue) {
    if(queue == NULL) return -1;
    if(queue->size == -1) return -1;
    #ifdef DEBUG
    printf("mqueue_destroy: Destruindo fila de mensagens\n");
    #endif
    free(queue->msgs);
    sem_destroy(&(queue->empty_spaces));
    sem_destroy(&(queue->buffer));
    sem_destroy(&(queue->msg));
    queue->size = -1;
    #ifdef DEBUG
    printf("mqueue_destroy: Fila de mensagens destruída\n");
    #endif
    return 0;
}

int mqueue_msgs (mqueue_t *queue) {
    if(queue == NULL) return -1;
    return queue->size;
}

void diskDriverBody (void * args) {
   while (true)
   {
      // obtém o semáforo de acesso ao disco

      // se foi acordado devido a um sinal do disco
      if (disco gerou um sinal)
      {
         // acorda a tarefa cujo pedido foi atendido
      }

      // se o disco estiver livre e houver pedidos de E/S na fila
      if (disco_livre && (fila_disco != NULL))
      {
         // escolhe na fila o pedido a ser atendido, usando FCFS
         // solicita ao disco a operação de E/S, usando disk_cmd()
      }

      // libera o semáforo de acesso ao disco

      // suspende a tarefa corrente (retorna ao dispatcher)
   }
}

int disk_mgr_init (int *num_blocks, int *block_size) {

}

int disk_block_read (int block, void *buffer) {
    // obtém o semáforo de acesso ao disco

    // inclui o pedido na fila_disco

    if (gerente de disco está dormindo)
    {
        // acorda o gerente de disco (põe ele na fila de prontas)
    }

    // libera semáforo de acesso ao disco

    // suspende a tarefa corrente (retorna ao dispatcher)
}

int disk_block_write (int block, void *buffer) {
    // obtém o semáforo de acesso ao disco

   // inclui o pedido na fila_disco

   if (gerente de disco está dormindo)
   {
      // acorda o gerente de disco (põe ele na fila de prontas)
   }

   // libera semáforo de acesso ao disco

   // suspende a tarefa corrente (retorna ao dispatcher)
}
