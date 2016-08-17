# PingPongOS
Descrição retirado da página do professor Carlos Maziero

## Sobre
Este projeto visa construir, de forma incremental, um pequeno sistema operacional didático. O sistema é construído inicialmente na forma de uma biblioteca de threads cooperativas dentro de um processo do sistema operacional real (Linux, MacOS ou outro Unix).

O desenvolvimento é incremental, adicionando gradativamente funcionalidades como preempção, contabilização, semáforos, filas de mensagens e acesso a um disco virtual. Essa abordagem simplifica o desenvolvimento e depuração do núcleo, além de dispensar o uso de linguagem de máquina.

A estrutura geral do código a ser desenvolvido é apresentada na figura abaixo. Os arquivos em azul são fixos (fornecidos pelo professor), enquanto os arquivos em verde devem ser desenvolvidos pelos alunos.

![Estrutura do PingPongOS](http://wiki.inf.ufpr.br/maziero/lib/exe/fetch.php?cache=&media=so:ppos.png)
