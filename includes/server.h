#ifndef _SERVER_H
#define _SERVER_H

#include "config.h"

#define MAX_CLIENTS 512

/* prototipo de función de atención a un socket 
 * compatible con el formato de los hilos de p_thread.
 * el parámetro de tipo void* será, por convención, 
 * un puntero a int, que será el file descriptor del socket.
 * es responsabilidad de la rutina liberar los argumentos */
typedef void* attention_routine(void*);

int server_setup(const char* servername, uint32_t local_addr, uint16_t local_port);
int server_setup_woptions(struct server_options *so);
int server_accept_loop(attention_routine *afn);

#endif /*_SERVER_H*/
