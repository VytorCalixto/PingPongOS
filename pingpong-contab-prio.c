// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016
//
// Teste da contabilização - tarefas com prioridades distintas

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t Pang, Peng, Ping, Pong, Pung ;

void Body (void * arg)
{
   int i,j ;

   printf ("%s INICIO em %4d ms (prio: %d)\n", (char *) arg,
           systime(), task_getprio(NULL)) ;

   for (i=0; i<40000; i++)
      for (j=0; j<40000; j++) ;

   printf ("%s FIM    em %4d ms\n", (char *) arg, systime()) ;
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   printf ("Main INICIO\n");

   ppos_init () ;

   task_create (&Pang, Body, "    Pang") ;
   task_setprio (&Pang, 0);

   task_create (&Peng, Body, "        Peng") ;
   task_setprio (&Peng, -2);

   task_create (&Ping, Body, "            Ping") ;
   task_setprio (&Ping, -4);

   task_create (&Pong, Body, "                Pong") ;
   task_setprio (&Pong, -6);

   task_create (&Pung, Body, "                    Pung") ;
   task_setprio (&Pung, -8);

   task_yield () ;

   printf ("Main FIM\n");
   exit (0);
}
