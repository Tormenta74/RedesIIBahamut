#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero
#include <sys/select.h>     // select

#include "globals.h"
#include "server.h"
#include "libdaemon.h"
#include "libtcp.h"
#include "libconcurrent.h"

int conn_socket;                // socket "maestro"
int active;                     // control del bucle del servidor
pthread_mutex_t nconn_lock;     // mutex del set de sockets
int n_conn;                     // número de clientes conectados
fd_set active_set, read_set;    // set de sockets y set de control

/* inicia el modo demonio, abre el socket de conexión llama a bind y a listen */
int server_setup(const char* servername, uint32_t local_addr, uint16_t local_port) {
    uint8_t *reader; // usado para leer correctamente la dirección IP
    int enable = 1;

    daemonize(servername);

    /* abrimos el socket de conexiones */
    conn_socket = tcp_open_socket();
    if(conn_socket == ERR) {
        print("Could not open TCP socket (%s:%d).", __FILE__, __LINE__);
        print("errno (socket): %s.", strerror(errno));
        return ERR;
    }
    print("Connection socket opened with file descriptor %d.\n", conn_socket);

    if(setsockopt(conn_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        print("Could not set socket options (%s:%d).", __FILE__, __LINE__);
        print("errno (setsockopt): %s.", strerror(errno));
        return ERR;
    }
    print("Socket options set (SO_REUSEADDR).");

    /* mostramos los datos de nuestro servidor */
    reader = (uint8_t*)&local_addr;
    print("Connection address: %d.%d.%d.%d", reader[0], reader[1], reader[2], reader[3]);
    print("Connection port: %d.", local_port);

    /* bind */
    if(tcp_bind_port(conn_socket, local_addr, local_port)) {
        print("Could not bind address to port (%s:%d).",__FILE__,__LINE__);
        print("errno (bind): %s.", strerror(errno));
        return ERR;
    }
    print("Conection port bound successfully.");

    /* escucha */
    if(tcp_listen(conn_socket, 128)) {
        print("Could not set socket to listen (%s:%d).", __FILE__, __LINE__);
        print("errno (listen): %s.", strerror(errno));
        return ERR;
    }

    if(mutex_init(&nconn_lock) == ERR) {
        print("Could not init mutex (%s:%d).", __FILE__, __LINE__);
        print("errno (pthread_mutex_init): %s.", strerror(errno));
        return ERR;
    }

    print("Now listening.");
    active = 1;
    return OK;
}

/* mientras "active" sea 1, el servidor se mantiene *
 * en un bucle de aceptar nuevas conexiones y       *
 * despachando hilos para atenderlas                */
int server_accept_loop(attention_routine *fn) {
    int new_socket;
    uint8_t *reader;
    struct sockaddr_in addr;


    while(active) {
        // inicializamos new_socket
        new_socket = 0;

        // inicializamos / limpiamos los campos de addr
        addr.sin_family = 0;
        addr.sin_port = 0;
        addr.sin_addr.s_addr = 0;
        bzero(addr.sin_zero, 8*sizeof(char));

        if(tcp_accept(conn_socket, &new_socket, &addr) || new_socket == ERR) {
            print("Could not accept conection request (%s:%d).", __FILE__, __LINE__);
            print("errno (accept): %s.", strerror(errno));
            active=0;
            return ERR;
        }

        // leemos los datos de la nueva conexión
        reader = (uint8_t*)&(addr.sin_addr.s_addr);
        print("New connection: address = %d.%d.%d.%d", reader[0], reader[1], reader[2], reader[3]);
        print("New connection: port = %d.", addr.sin_port);
        print("Conection accepted: redirected to socket %d.", new_socket);


        // lanzamos el hilo de atención, pasándole el número del socket
        int* s = malloc(sizeof(int));
        *s = new_socket;

        mutex_lock(&nconn_lock);
        if(++n_conn <= MAX_CLIENTS) {
            conc_launch(fn, (void*)s);
        } else {
            // TODO: either nothing, or send a message to the client informing them
            // that we are not accepting new connections
        }
        mutex_unlock(&nconn_lock);
    }
}

/* mientras "active" sea 1, el servidor se mantiene *
 * en un bucle de aceptar nuevas conexiones y       *
 * despachando hilos para atenderlas                */
int server_accept_loop_old(attention_routine *fn) {
    int i, new_socket;
    uint8_t *reader;
    struct sockaddr_in addr;
    //pthread_t thread;

    // inicializamos los campos de addr
    addr.sin_family=0;
    addr.sin_port=0;
    addr.sin_addr.s_addr=0;
    bzero(addr.sin_zero,8*sizeof(char));

    while(active) {
        mutex_lock(&nconn_lock);
        read_set = active_set;
        mutex_unlock(&nconn_lock);

        print("Selecting.");

        if(select(n_conn, &read_set, NULL, NULL, NULL) < 0) {
            print("Error while listening to the sockets (%s:%d).", __FILE__, __LINE__);
            print("errno (select): %s.", strerror(errno));
            active = 0;
            return ERR;
        }

        print("He salío de select.");

        for(i=0; i<FD_SETSIZE; i++) {
            if(FD_ISSET(i,&read_set) && i == conn_socket) {
                new_socket=0;

                if(tcp_accept(conn_socket,&new_socket,&addr) || new_socket == ERR) {
                    print("Could not accept conection request (%s:%d).", __FILE__, __LINE__);
                    print("errno (accept): %s.", strerror(errno));
                    active=0;
                    return ERR;
                }
                reader=(uint8_t*)&(addr.sin_addr.s_addr);
                print("New connection: address = %d.%d.%d.%d", reader[0], reader[1], reader[2], reader[3]);
                print("New connection: port = %d.", addr.sin_port);
                print("Conection accepted: redirected to socket %d.", new_socket);

                mutex_lock(&nconn_lock);
                FD_SET(new_socket,&active_set);
                mutex_unlock(&nconn_lock);
                n_conn = new_socket+1;

                addr.sin_family = 0;
                addr.sin_port = 0;
                addr.sin_addr.s_addr = 0;
                bzero(addr.sin_zero, 8*sizeof(char));
            }
            else if(FD_ISSET(i, &read_set)) {
                int* s = malloc(sizeof(int));
                *s = i;
                conc_launch(fn,(void*)s);
            }
        }
    }
}
