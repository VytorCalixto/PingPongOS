// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016
//
// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__
#define STACKSIZE 32768
#include <ucontext.h>

// Task status
#define FINISHED 0
#define READY 1
#define SUSPENDED 2
#define RUNNING 3

// Task "type"
#define SYSTEM_TASK 0
#define USER_TASK 1

#define SYSTEM_TICKS 20

// Estrutura que define uma tarefa
typedef struct task_t
{
    struct task_t *prev, *next ;   // para usar com a biblioteca de filas (cast)
    int tid ;                      // ID da tarefa
    ucontext_t context;
    int status; // Task Status
    int priority;
    int aging; // Dynamic priority
    int type; // Task type (system or user)
    // Time
    int ticks;
    unsigned int exe_init_time, exe_end_time, proc_time, activations;
    int returnValue;
    struct task_t *joinQueue;
    unsigned int wakeup_time;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
    int lock;
    int value;
    task_t *queue;
    int live;
    int returnValue;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
    // preencher quando for necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando for necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
    int size;
    int max_msgs;
    int msg_size;
    int head, tail;
    void **msgs;
    semaphore_t empty_spaces;
    semaphore_t buffer;
    semaphore_t msg;
} mqueue_t ;

#endif
