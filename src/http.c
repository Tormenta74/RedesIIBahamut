#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero
#include <unistd.h>         // getpid

#include "globals.h"
#include "config.h"
#include "server.h"
#include "libconcurrent.h"
#include "libtcp.h"

extern int active;                  // server.c
extern pthread_mutex_t nconn_lock;  // server.c
extern int n_conn;                  // server.c

void *echo (void* args) {
    int sock = *(int*)args;
    int len = -1;
    char buffer[1024];
    print("echo: handling socket %d.", sock);
    free(args); // done

    if(sock <= 0) {
        print("Wrong parameters for socket. (%s:%d).", __FILE__, __LINE__);
        conc_exit(NULL);
    }

    bzero(buffer, 1024);

    if((len = tcp_receive(sock, buffer, 1024)) == 0) {
        print("Client closing connection.");
        tcp_close_socket(sock);

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    if(len < 0) {
        print("Could not receive any data (%s:%d).", __FILE__, __LINE__);
        print("errno (receive): %s.", strerror(errno));

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    print("Received %d bytes.", len);

    if(tcp_send(sock, (const void*)buffer, strlen(buffer)) < 0) {
        print("could not send pong back (%s:%d).", __FILE__, __LINE__);
        print("errno (send): %s.", strerror(errno));

        mutex_lock(&nconn_lock);
        n_conn--;
        mutex_unlock(&nconn_lock);

        conc_exit();
    }

    print("Echo correctly sent back. Bye now!");
    tcp_close_socket(sock);

    mutex_lock(&nconn_lock);
    n_conn--;
    mutex_unlock(&nconn_lock);

    conc_exit();
    return OK;
}

int main(int argc, char *argv[]) {
    int status = ERR;
    struct server_options so;

    // Opcional: pasando como parámetros "--config path/to/my/config/file"
    // podemos cambiar el fichero de configuración. De lo contrario, usamos
    // server.conf, en la raíz
    if(argc != 1) {
        if(strcmp(argv[1], "--config") != 0) {
            fprintf(stderr, "Unrecognized option: %s\n", argv[1]);
            return ERR;
        } else {
            printf("Loading alternative configuration file.\n");
            status = config_parse(argv[2], &so);
        }
    } else {
        status = config_parse("server.conf", &so);
    }

    printf("Configuration:\n");
    printf("pid: %d\n", getpid());
    config_print(&so);

    if(status == ERR) {
        printf("Bad server options\n");
        return ERR;
    }

    status = server_setup_woptions(&so);
    if(status == ERR) {
        return ERR;
    }

    status = server_accept_loop(echo);

    return status;
}

