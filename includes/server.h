#ifndef _SERVER_H
#define _SERVER_H

#include "config.h"

#define MAX_CLIENTS     512
#define MAX_RECV_LEN    3*1024

/* Prototipo de función de atención a un socket
 * compatible con el formato de los hilos de p_thread.
 * El parámetro de tipo void* será, por convención,
 * un puntero a int, que será el file descriptor del socket.
 * Es responsabilidad de la rutina liberar los argumentos */
typedef void* attention_routine(void*);

int server_setup(struct server_options *so);
int server_accept_loop(attention_routine *afn);

#endif /*_SERVER_H*/
