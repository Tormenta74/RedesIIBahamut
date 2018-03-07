#include <arpa/inet.h>      // INADDR_ANY
#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <signal.h>         // signal
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero
#include <sys/select.h>     // select
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include "globals.h"
#include "config.h"
#include "server.h"
#include "libdaemon.h"
#include "libtcp.h"
#include "libconcurrent.h"

int conn_socket;                // socket "maestro"
int active;                     // control del bucle del servidor
pthread_mutex_t nconn_lock;     // mutex del número de conexiones
int n_conn;                     // número de clientes conectados
int iter = 0;                   // bandera de modo iterativo

int control[2];                 // file descriptor de control
int max_control_socket;         // el máximo entre conn y closing
fd_set a_set, r_set;            // fd_sets que contendrán a los dos sockets principales
//     (conn y closing)

#define TERMINATE_R control[0]
#define TERMINATE_W control[1]


/*
 * Description: Catches SIGINT and indicates the accept loop to end.
 *
 * In:
 * int sig_no: signal number
 */
void handler(int sig_no) {
    print("Server terminated: %d captured.", sig_no);
    // avoid new interactions that are not already in process
    // also signal the active clients that no further interactions will be processed
    active = 0;
    // trigger the select call
    write(TERMINATE_W, "Dewit", 6);
}

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
    int enable = 1, status;

    // sanity check
    if (!so) {
        print("libserver: Unallocated server_options.");
        return ERR;
    }

    // daemon mode option
    if (so->daemon == 1) {
        if (daemonize(so->server_signature, so->server_root) == ERR) {
            print("libserver: Bad daemonize.");
            return ERR;
        }
    } else {
        // file creation permissions 0000
        umask(0);
        // set log priorities
        setlogmask(LOG_UPTO(LOG_INFO));
        // open the log
        openlog(so->server_signature, LOG_CONS|LOG_PID|LOG_NDELAY, LOG_LOCAL3);
    }

    // signal handling

    struct sigaction sa;

    // setting options
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    // calls to sigaction
    if (sigaction(SIGINT, &sa, NULL) == ERR
            || sigaction(SIGTERM, &sa, NULL) == ERR) {
        print("libserver: Could not set signal handler (%s:%d).", __FILE__, __LINE__);
        exit(ERR);
    }

    print("libserver: SIGINT and SIGTERM handlers set.\n");

    // open the master (connection) socket
    conn_socket = tcp_open_socket();
    if (conn_socket == ERR) {
        print("libserver: Could not open TCP socket (%s:%d).", __FILE__, __LINE__);
        print("libserver: errno (socket): %s.", strerror(errno));
        return ERR;
    }
    print("libserver: Connection socket opened with file descriptor %d.\n", conn_socket);

    // open the control socket
    status = pipe(control);
    if (status < 0) {
        print("libserver: Could not open TCP socket (%s:%d).", __FILE__, __LINE__);
        print("libserver: errno (socket): %s.", strerror(errno));
        return ERR;
    }

    // setting the max descriptor number for select
    if (TERMINATE_R > conn_socket) {
        max_control_socket = TERMINATE_R + 1;
    } else {
        max_control_socket = conn_socket + 1;
    }

    // apply socket options
    if (setsockopt(conn_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        print("libserver: Could not set socket options (%s:%d).", __FILE__, __LINE__);
        print("libserver: errno (setsockopt): %s.", strerror(errno));
        return ERR;
    }
    print("libserver: Socket options set (SO_REUSEADDR).");

    // relevant information to the testers
    print("libserver: Connection port: %d.", so->listen_port);

    // bind the master socket to the listen port
    if (tcp_bind_port(conn_socket, INADDR_ANY, so->listen_port)) {
        print("libserver: Could not bind address to port (%s:%d).", __FILE__, __LINE__);
        print("libserver: errno (bind): %s.", strerror(errno));
        return ERR;
    }
    print("libserver: Conection port bound successfully.");

    // mark socket as ready to accept new connections
    if (tcp_listen(conn_socket, so->max_clients)) {
        print("libserver: Could not set socket to listen (%s:%d).", __FILE__, __LINE__);
        print("libserver: errno (listen): %s.", strerror(errno));
        return ERR;
    }

    // init locks
    if (mutex_init(&nconn_lock) == ERR) {
        print("libserver: Could not init mutexes (%s:%d).", __FILE__, __LINE__);
        print("libserver: errno (pthread_mutex_init): %s.", strerror(errno));
        return ERR;
    }

    // initialize sets
    FD_ZERO(&a_set);

    // include both sockets in the set
    FD_SET(conn_socket, &a_set);
    FD_SET(TERMINATE_R, &a_set);

    // copy
    r_set = a_set;

    // ready
    print("libserver: Now listening.");

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
    int new_socket, status;
    uint8_t *reader;
    struct sockaddr_in addr;

    while (active) {
        // inicializamos new_socket
        new_socket = 0;

        r_set = a_set;

        // inicializamos / limpiamos los campos de addr
        addr.sin_family = 0;
        addr.sin_port = 0;
        addr.sin_addr.s_addr = 0;
        bzero(addr.sin_zero, 8*sizeof(char));

        /*** replacement: directly accepting -> selecting and then accepting ***/

        // select on the main sockets
        status = select(max_control_socket, &r_set, NULL, NULL, NULL);
        if (status < 0) {
            print("libserver: Error on select (%s:%d).", __FILE__, __LINE__);
            print("libserver: errno (accept): %s.", strerror(errno));
            return ERR;
        }

        // selected on the closing socket
        // (this is performed first, as if the signal is given
        // the server shall accept no more connections)
        if (FD_ISSET(TERMINATE_R, &r_set)) {
            print("libserver: Control socket activated. Leaving main loop.");
            return OK;
        }

        // at this point, closing socket was not activated
        // therefore next check should never trigger, as conn_socket
        // is the only other file descriptor in the sets

        if (!FD_ISSET(conn_socket, &r_set)) {
            print("libserver: Select did not trigger either on either of the sockets.");
            return ERR;
        }

        // at this point, we know the connection socket was activated, so we proceed to accept
        // this will be non-blocking
        if (tcp_accept(conn_socket, &new_socket, &addr) || new_socket == ERR) {
            print("libserver: Could not accept conection request (%s:%d).", __FILE__, __LINE__);
            print("libserver: errno (accept): %s.", strerror(errno));
            return ERR;
        }

        // leemos los datos de la nueva conexión
        reader = (uint8_t*)&(addr.sin_addr.s_addr);
        print("libserver: New connection: address = %d.%d.%d.%d", reader[0], reader[1], reader[2], reader[3]);
        print("libserver: New connection: port = %d.", addr.sin_port);
        print("libserver: Conection accepted: redirected to socket %d.", new_socket);


        // lanzamos el hilo de atención, pasándole el número del socket
        int* s = malloc(sizeof(int));
        *s = new_socket;

        // incrementamos y después evaluamos: todo ha ido bien
        mutex_lock(&nconn_lock);
        if (++n_conn <= MAX_CLIENTS) {
            // tenemos que desbloquear inmediatamente (it is known)
            mutex_unlock(&nconn_lock);
            if (iter == 1) { // el servidor tiene la opción de ser iterativo
                // llamamos directamente a la rutina de atención
                fn((void*)s);
            } else {
                // lanzamos concurrentemente la rutina de atención
                conc_launch(fn, (void*)s);
            }
        } else {
            // tenemos que desbloquear inmediatamente (it is known)
            mutex_unlock(&nconn_lock);
            // TODO: either nothing, or send a message to the client informing them
            // that we are not accepting new connections
            tcp_close_socket(new_socket);
            free(s);
        }
    }

    return OK;
}


/************************************************
 * DEPRECATED
 ************************************************/

int server_setup_old(const char* servername, uint32_t local_addr, uint16_t local_port) {
    uint8_t *reader; // usado para leer correctamente la dirección IP
    int enable = 1;

    //daemonize(servername);

    /* abrimos el socket de conexiones */
    conn_socket = tcp_open_socket();
    if (conn_socket == ERR) {
        print("Could not open TCP socket (%s:%d).", __FILE__, __LINE__);
        print("errno (socket): %s.", strerror(errno));
        return ERR;
    }
    print("Connection socket opened with file descriptor %d.\n", conn_socket);

    if (setsockopt(conn_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
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
    if (tcp_bind_port(conn_socket, local_addr, local_port)) {
        print("Could not bind address to port (%s:%d).", __FILE__, __LINE__);
        print("errno (bind): %s.", strerror(errno));
        return ERR;
    }
    print("Conection port bound successfully.");

    /* escucha */
    if (tcp_listen(conn_socket, 128)) {
        print("Could not set socket to listen (%s:%d).", __FILE__, __LINE__);
        print("errno (listen): %s.", strerror(errno));
        return ERR;
    }

    if (mutex_init(&nconn_lock) == ERR) {
        print("Could not init mutex (%s:%d).", __FILE__, __LINE__);
        print("errno (pthread_mutex_init): %s.", strerror(errno));
        return ERR;
    }

    print("Now listening.");
    active = 1;
    return OK;
}

// for the sigaction and sigprocmask:
// https://stackoverflow.com/questions/6962150/catching-signals-while-reading-from-pipe-with-select
