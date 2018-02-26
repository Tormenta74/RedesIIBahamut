#include <errno.h>          // errno
#include <pthread.h>        // pthread_mutex_t
#include <stdint.h>         // uintXX_t
#include <stdio.h>
#include <stdlib.h>         // malloc
#include <string.h>         // strerror
#include <strings.h>        // bzero
#include <unistd.h>         // getpid

#include "globals.h"
#include "picohttpparser.h"
#include "parser.h"

int main(int argc, char *argv[]) {
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
