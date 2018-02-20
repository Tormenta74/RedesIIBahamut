#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero

#include "globals.h"
#include "config.h"
#include "server.h"
#include "libconcurrent.h"
#include "libtcp.h"
#include "picohttpparser.h"

//extern int active;  // server.c

//void *http(void *args) {
//    int sock = *(int*)args;
//    int len = -1;
//    char buffer[1024];
//    print("file: handdling socket %d.", sock);
//    free(args); // done
//
//    if(sock <= 0) {
//        print("Wrong parameters for socket. (%s:%d).", __FILE__, __LINE__);
//        conc_exit(NULL);
//    }
//
//    bzero(buffer, 1024);
//
//    if((len = tcp_receive(sock, buffer, 1024)) == 0) {
//        print("Client closing connection.");
//        tcp_close_socket(sock);
//
//        conc_exit();
//    }
//
//    if(len < 0) {
//        print("Could not receive any data (%s:%d).", __FILE__, __LINE__);
//        print("errno (receive): %s.", strerror(errno));
//        conc_exit();
//    }
//
//    print("Received %d bytes.", len);
//
//    // PARSE REQUEST
//
//    tcp_close_socket(sock);
//    conc_exit();
//}

int main(int argc, char *argv[]) {
    //uint16_t local_port = 8000;
    //uint32_t local_addr = INADDR_ANY;
    int status;
    struct server_options so;

    //printf("Going down: use '$ journalctl -f | grep http_server' to follow the server logs.\n");

    //status = server_setup("http_server", local_addr, local_port);
    //if(status == ERR) {
    //    print("Error while setting up server. Shutting down.");
    //    return ERR;
    //}

    //status = server_accept_loop(http);

    status = config_parse("httprc", &so);

    return status;
}

