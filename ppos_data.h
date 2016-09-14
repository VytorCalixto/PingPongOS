// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016
//
// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__
#define STACKSIZE 32768
#include <ucontext.h>

#define FINISHED 0
#define READY 1
#define SUSPENDED 2
#define RUNNING 3

// Estrutura que define uma tarefa
typedef struct task_t
{
    struct task_t *prev, *next ;   // para usar com a biblioteca de filas (cast)
    int tid ;                      // ID da tarefa
    ucontext_t context;
    int status;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando for necessário
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
  // preencher quando for necessário
} mqueue_t ;

#endif
