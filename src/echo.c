#include <arpa/inet.h>  // INADDR_ANY
#include <errno.h>      // errno
#include <signal.h>     // signal
#include <stdint.h>     // uintXX_t
#include <stdio.h>
#include <stdlib.h>     // free
#include <string.h>     // strerror
#include <strings.h>    // bzero

#include "globals.h"
#include "server.h"
#include "libconcurrent.h"
#include "libtcp.h"

extern int conn_socket;                 // server.c
extern int active;                      // server.c
extern pthread_mutex_t fd_set_lock;     // server.c
extern fd_set active_set;               // server.c

void handleSIGINT(int sig_no) {
    print("Server terminated: SIGINT captured.");
    active = 0;
}

void *echo (void* args) {
    int sock = *(int*)args;
    int len = -1;
    char buffer[1024];
    print("echo: handling socket %d.", sock);

    if(sock <= 0) {
        print("Wrong parameters for socket. (%s:%d).", __FILE__, __LINE__);
        conc_exit(NULL);
    }

    bzero(buffer, 1024);

    if((len = tcp_receive(sock, buffer, 1024)) == 0) {
        print("Client closing connection.");
        tcp_close_socket(sock);

        mutex_lock(&fd_set_lock);
        FD_CLR(sock, &active_set);
        mutex_unlock(&fd_set_lock);

        conc_exit();
    }

    if(len < 0) {
        print("Could not receive any data (%s:%d).", __FILE__, __LINE__);
        print("errno (receive): %s.", strerror(errno));
        conc_exit();
    }

    print("Received %d bytes.", len);

    if(tcp_send(sock, (const void*)buffer, strlen(buffer)) < 0) {
        print("could not send pong back (%s:%d).", __FILE__, __LINE__);
        print("errno (send): %s.", strerror(errno));
        conc_exit();
    }

    print("Echo correctly sent back. Bye now!");
    free(args);
    conc_exit();
}

int main(int argc, char *argv[]) {
    uint16_t local_port = 8000;
    uint32_t local_addr = INADDR_ANY;
    int status;

    /* Ejemplo de uso de las funciones de server */

    if((signal(SIGINT, handleSIGINT)) == SIG_ERR) {
        printf("Could not set signal handler (%s:%d).", __FILE__, __LINE__);
        exit(ERR);
    }
    printf("SIGINT handler set.");

    printf("Going down as 'echo_server'.\n");

    status = server_setup("echo_server", local_addr, local_port);
    if(status == ERR) {
        print("Error while setting up server. Shutting down.");
        return ERR;
    }
    print("active = %d.", active);
    status = server_accept_loop(echo);
    return status;
}
