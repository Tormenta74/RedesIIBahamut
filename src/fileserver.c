#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero

#include "globals.h"
#include "server.h"
#include "libconcurrent.h"
#include "libtcp.h"

extern int active;  // server.c

pthread_mutex_t file_lock;
char *file_contents;
long filelen;

void *file(void *args) {
    int sock = *(int*)args;
    int len = -1;
    char buffer[1024];
    print("file: handdling socket %d.", sock);
    free(args); // done

    if(sock <= 0) {
        print("Wrong parameters for socket. (%s:%d).", __FILE__, __LINE__);
        conc_exit(NULL);
    }

    bzero(buffer, 1024);

    if((len = tcp_receive(sock, buffer, 1024)) == 0) {
        print("Client closing connection.");
        tcp_close_socket(sock);

        conc_exit();
    }

    if(len < 0) {
        print("Could not receive any data (%s:%d).", __FILE__, __LINE__);
        print("errno (receive): %s.", strerror(errno));
        conc_exit();
    }

    print("Received %d bytes.", len);

    mutex_lock(&file_lock);
    if(tcp_send(sock, (const void*)file_contents, filelen) < 0) {
        mutex_unlock(&file_lock);
        print("could not send file (%s:%d).", __FILE__, __LINE__);
        print("errno (send): %s.", strerror(errno));
        conc_exit();
    }
    mutex_unlock(&file_lock);

    print("Echo correctly sent back. Bye now!");
    tcp_close_socket(sock);
    conc_exit();
}

int main(int argc, char *argv[]) {
    uint16_t local_port = 8000;
    uint32_t local_addr = INADDR_ANY;
    int status;
    FILE *fp;

    if(argc != 2) {
        fprintf(stderr, "Usage: ./fileserver <file>\n");
        exit(ERR);
    }

    // get the file from the arguments to the program
    if((fp = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Could not read file %s.\n", argv[1]);
        exit(ERR);
    }

    // calculate file size
    fseek(fp, 0, SEEK_END);
    filelen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // reserve enough memory for the entire file and load it
    file_contents = (char*)malloc(filelen+1);
    fread(file_contents, filelen, 1, fp);
    file_contents[filelen] = '\0';

    // init the mutex for the copy of the file
    if(mutex_init(&file_lock) == ERR) {
        printf("Could not init file mutex.\n");
        return ERR;
    }

    printf("Going down: use '$ journalctl -f | grep file_server' to follow the server logs.\n");

    status = server_setup("file_server", local_addr, local_port);
    if(status == ERR) {
        print("Error while setting up server. Shutting down.");
        return ERR;
    }

    status = server_accept_loop(file);

    return status;
}
