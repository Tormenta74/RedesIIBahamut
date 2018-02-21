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
#include "parser.h"

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
    //int status;
    //struct server_options so;

    //printf("Going down: use '$ journalctl -f | grep http_server' to follow the server logs.\n");

    //status = server_setup("http_server", local_addr, local_port);
    //if(status == ERR) {
    //    print("Error while setting up server. Shutting down.");
    //    return ERR;
    //}

    //status = server_accept_loop(http);

    //status = config_parse("server.conf", &so);

    char buf[2048], *method, *path;
    int ret, minor_version, i;
    size_t method_len, path_len, num_headers;
    struct phr_header headers[100];

    sprintf(buf, "GET /somedir/page.html HTTP/1.1\r\nHost: www.someschool.edu\r\nUser-Agent: Mozilla/4.0\r\nConnection: close\r\nAccept-language: fr\r\n\r\n");
    num_headers = (sizeof(headers) / sizeof(headers[0]));
    ret = phr_parse_request(buf, strlen(buf), (const char **) &method, &method_len, (const char **) &path, &path_len, &minor_version, headers, &num_headers, 0);

    if (ret > 0) {
      printf("Parsed successful.\n");
   } else if (ret == -1 || ret == -2) {
      printf("Parse error.\n");
   }

   printf("request is %d bytes long\n", ret);
   printf("method is %.*s\n", (int)method_len, method);
   printf("path is %.*s\n", (int)path_len, path);
   printf("HTTP version is 1.%d\n", minor_version);
   printf("numheaders: %d ; headers:\n", (int) num_headers);
   for (i = 0; i != num_headers; ++i) {
       printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
              (int)headers[i].value_len, headers[i].value);
   }
    return 0;
}
