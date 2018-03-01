#include <arpa/inet.h>      // INADDR_ANY
#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero
#include <sys/select.h>     // select

#include "globals.h"
#include "config.h"
#include "server.h"
#include "libdaemon.h"
#include "libtcp.h"
#include "libconcurrent.h"

int conn_socket;                // socket "maestro"
int active;                     // control del bucle del servidor
pthread_mutex_t nconn_lock;     // mutex del set de sockets
int n_conn;                     // número de clientes conectados
int iter = 0;                   // bandera de modo iterativo
fd_set active_set, read_set;    // set de sockets y set de control

/*
 * Description: Establishes options for the server and sets the master socket.
 *
 * In:
 * struct server_options *so: struct containing the loaded options
 * (see includes/config.h for details)
 *
 * Return: ERR in case of failure at any point. OK otherwise.
 */
int server_setup(struct server_options *so) {
    int enable = 1;

    // sanity check
    if(!so) {
        print("Unallocated server_options.");
        return ERR;
    }

    // daemon mode option
    if(so->daemon == 1) {
        if(daemonize(so->server_signature, so->server_root) == ERR) {
            print("Bad daemonize.");
            return ERR;
        }
    }

    // open the master (connection) socket
    conn_socket = tcp_open_socket();
    if(conn_socket == ERR) {
        print("Could not open TCP socket (%s:%d).", __FILE__, __LINE__);
        print("errno (socket): %s.", strerror(errno));
        return ERR;
    }
    print("Connection socket opened with file descriptor %d.\n", conn_socket);

    // apply socket options
    if(setsockopt(conn_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        print("Could not set socket options (%s:%d).", __FILE__, __LINE__);
        print("errno (setsockopt): %s.", strerror(errno));
        return ERR;
    }
    print("Socket options set (SO_REUSEADDR).");

    // relevant information to the testers
    print("Connection port: %d.", so->listen_port);

    // bind the master socket to the listen port
    if(tcp_bind_port(conn_socket, INADDR_ANY, so->listen_port)) {
        print("Could not bind address to port (%s:%d).",__FILE__,__LINE__);
        print("errno (bind): %s.", strerror(errno));
        return ERR;
    }
    print("Conection port bound successfully.");

    // mark socket as ready to accept new connections
    if(tcp_listen(conn_socket, so->max_clients)) {
        print("Could not set socket to listen (%s:%d).", __FILE__, __LINE__);
        print("errno (listen): %s.", strerror(errno));
        return ERR;
    }

    // init number of connections variable lock
    if(mutex_init(&nconn_lock) == ERR) {
        print("Could not init mutex (%s:%d).", __FILE__, __LINE__);
        print("errno (pthread_mutex_init): %s.", strerror(errno));
        return ERR;
    }

    print("Now listening.");

    // activate the server
    active = 1;

    // option to either launch threads for each client or execute iteratively
    iter = so->iterative;

    return OK;
}

/*
 * Description: While the "active" global variable is set to 1, accepts new
 *              connections and launches threads to handle each client.
 *
 * In:
 * attention_routine *fn: pointer to a function that handles a client connection
 * (see includes/server.h for details)
 *
 * Return: ERR in case of failure at any point. OK otherwise.
 */
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
            if(iter == 1) { // el servidor tiene la opción de ser iterativo
                // llamamos directamente a la rutina de atención
                fn((void*)s);
            } else {
                // lanzamos concurrentemente la rutina de atención
                conc_launch(fn, (void*)s);
            }
        } else {
            // TODO: either nothing, or send a message to the client informing them
            // that we are not accepting new connections
            tcp_close_socket(new_socket);
            free(s);
        }
        mutex_unlock(&nconn_lock);
    }

    return OK;
}


/************************************************
 * DEPRECATED
 */

int server_setup_old(const char* servername, uint32_t local_addr, uint16_t local_port) {
    uint8_t *reader; // usado para leer correctamente la dirección IP
    int enable = 1;

    //daemonize(servername);

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

    return OK;
}

