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

   char buf[MAX_CHAR], method[MAX_CHAR], path[MAX_CHAR];
   int ret, version, i, num_headers;
   struct request_headers headers[MAX_HEADERS];

   sprintf(buf, "GET /somedir/page.html HTTP/1.1\r\nHost: www.someschool.edu\r\nUser-Agent: Mozilla/4.0\r\nConnection: close\r\nAccept-language: fr\r\n\r\n");
   ret = request_parser(buf, strlen(buf), method, path, &version, headers, &num_headers);

   if (ret == ERR) {
      printf("Error.\n");
      return -1;
   }

   printf("method is %s\n", method);
   printf("path is %s\n", path);
   printf("HTTP version is 1.%d\n", version);
   printf("numheaders: %d ; headers:\n", num_headers);
   for (i = 0; i < num_headers; i++) {
       printf("%s: %s\n", headers[i].name, headers[i].value);
   }

   return 0;
}
