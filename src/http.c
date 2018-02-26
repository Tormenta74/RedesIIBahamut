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

int process_request(char *buf, size_t buflen, char *response) {
   char method[MAX_CHAR], path[MAX_CHAR], resp[MAX_CHAR], body[MAX_CHAR];
   int ret, version, i, num_req_headers, num_res_headers, rescode;
   struct http_headers req_headers[MAX_HEADERS], res_headers[MAX_HEADERS];

   ret = request_parser(buf, buflen, method, path, &version, req_headers, &num_req_headers);

   if (ret == ERR) {
      print("Error when processing request.\n");
      return ERR;
   }

   if (strcmp(method, "OPTIONS")==0) {
      sprintf(res_headers[0].name, "Allow");
      sprintf(res_headers[0].value, "OPTIONS, GET, POST");
      sprintf(res_headers[1].name, "Date");
      sprintf(res_headers[1].value, "Fri, 09 May 2012 18:54:21 GTM");
      sprintf(res_headers[2].name, "Server");
      sprintf(res_headers[2].value, "Diego, you gotta fix this thing right here");
      ret = response_builder(response, version, 200, "OK", 2, 3, res_headers, NULL, 0);

      if (ret == ERR) {
         print("Error when creating OPTIONS response.\n");
         return ERR;
      }

      return OK;
   }

}

int main(int argc, char *argv[]) {

   char buf[MAX_CHAR], method[MAX_CHAR], path[MAX_CHAR], resp[MAX_CHAR], body[MAX_CHAR], response[MAX_CHAR];
   int ret, version, i, num_headers, rescode;
   struct http_headers headers[MAX_HEADERS];

   //sprintf(buf, "GET /somedir/page.html HTTP/1.1\r\nHost: www.someschool.edu\r\nUser-Agent: Mozilla/4.0\r\nConnection: close\r\nAccept-language: fr\r\n\r\n");
   //ret = request_parser(buf, strlen(buf), method, path, &version, headers, &num_headers);

   sprintf(buf, "HTTP/1.1 200 OK\r\nDate: Wed, 13 May 2009 16:25:12 GMT\r\nExpires: Tue, 12 May 2009 16:25:12 GMT\r\nContent-Length: 40\r\nContent-Type: text/xml\r\n\r\n");
   ret = response_parser(buf, strlen(buf), &version, &rescode, resp, headers, &num_headers);

   if (ret == ERR) {
      printf("Error.\n");
      return ERR;
   }

   //printf("method is %s\n", method);
   //printf("path is %s\n", path);
   printf("message is %s\n", resp);
   printf("response code is %d\n", rescode);
   printf("HTTP version is 1.%d\n", version);
   printf("numheaders: %d ; headers:\n", num_headers);
   for (i = 0; i < num_headers; i++) {
      printf("%s: %s\n", headers[i].name, headers[i].value);
   }

   strcpy(body, "Hola mundo.\n");

   ret = response_builder(buf, version, rescode, resp, strlen(resp), num_headers, headers, body, strlen(body));

   if (ret == ERR) {
      printf("Error.\n");
      return ERR;
   }

   printf("\n%s\n\n", buf);

   sprintf(buf, "OPTIONS /somedir/page.html HTTP/1.1\r\n\r\n");
   ret = process_request(buf, strlen(buf), response);

   if (ret == ERR) {
      printf("Error.\n");
      return ERR;
   }

   printf("\n%s\n\n", response);

   return OK;
}
