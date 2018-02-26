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
#include "picohttpparser.h"
#include "headers.h"
#include "parser.h"

struct server_options so;

int process_request(char *buf, size_t buflen, char *response) {
    char *date_buf;
    int ret;
    struct http_headers res_headers[MAX_HEADERS];
    struct http_req_data rd;

    ret = request_parser_new(buf, buflen, &rd);

    request_data_print(&rd);

    if(ret == ERR) {
        print("Error when processing request.\n");
        return ERR;
    }

    date_buf = header_date();

    // GET
    //if(strcmp(method, "GET") == 0) {

    // POST
    //if(strcmp(method, "POST") == 0) {

    // OPTIONS
    if(strcmp(rd.method, "OPTIONS") == 0) {
        sprintf(res_headers[0].name, "Allow");
        sprintf(res_headers[0].value, "OPTIONS, GET, POST");
        sprintf(res_headers[1].name, "Date");
        sprintf(res_headers[1].value, date_buf);
        sprintf(res_headers[2].name, "Server");
        sprintf(res_headers[2].value, so.server_signature);

        ret = response_builder(response, rd.version, 200, "OK", 2, 3, res_headers, NULL, 0);

        if(ret == ERR) {
            print("Error while creating OPTIONS response.\n");
            free(date_buf);
            return ERR;
        }

        free(date_buf);
        return OK;
    }

    return OK;
}

//int main(int argc, char *argv[]) {
//
//    char buf[MAX_CHAR], method[MAX_CHAR], path[MAX_CHAR], resp[MAX_CHAR], body[MAX_CHAR], response[MAX_CHAR];
//    int ret, version, i, num_headers, rescode;
//    struct http_headers headers[MAX_HEADERS];
//
//    //sprintf(buf, "GET /somedir/page.html HTTP/1.1\r\nHost: www.someschool.edu\r\nUser-Agent: Mozilla/4.0\r\nConnection: close\r\nAccept-language: fr\r\n\r\n");
//    //ret = request_parser(buf, strlen(buf), method, path, &version, headers, &num_headers);
//
//    sprintf(buf, "HTTP/1.1 200 OK\r\nDate: Wed, 13 May 2009 16:25:12 GMT\r\nExpires: Tue, 12 May 2009 16:25:12 GMT\r\nContent-Length: 40\r\nContent-Type: text/xml\r\n\r\n");
//    ret = response_parser(buf, strlen(buf), &version, &rescode, resp, headers, &num_headers);
//
//    if (ret == ERR) {
//        printf("Error.\n");
//        return ERR;
//    }
//
//    //printf("method is %s\n", method);
//    //printf("path is %s\n", path);
//    printf("message is %s\n", resp);
//    printf("response code is %d\n", rescode);
//    printf("HTTP version is 1.%d\n", version);
//    printf("numheaders: %d ; headers:\n", num_headers);
//    for (i = 0; i < num_headers; i++) {
//        printf("%s: %s\n", headers[i].name, headers[i].value);
//    }
//
//    strcpy(body, "Hola mundo.\n");
//
//    ret = response_builder(buf, version, rescode, resp, strlen(resp), num_headers, headers, body, strlen(body));
//
//    if (ret == ERR) {
//        printf("Error.\n");
//        return ERR;
//    }
//
//    printf("\n%s\n\n", buf);
//
//    sprintf(buf, "OPTIONS /somedir/page.html HTTP/1.1\r\n\r\n");
//    ret = process_request(buf, strlen(buf), response);
//
//    if (ret == ERR) {
//        printf("Error.\n");
//        return ERR;
//    }
//
//    printf("\n%s\n\n", response);
//
//    return OK;
//}

int main() {
    int ret;
    char buf[MAX_CHAR], response[MAX_CHAR];

    // config related

    ret = config_parse("server.conf", &so);
    if(ret == ERR) {
        printf("Parsing failed.\n");
        exit(ERR);
    }


    // http parsing related

    // change your request here
    //sprintf(buf, "GET /somedir/page.html HTTP/1.1\r\nHost: www.someschool.edu\r\nUser-Agent: Mozilla/4.0\r\nConnection: close\r\nAccept-language: fr\r\n\r\n");
    sprintf(buf, "OPTIONS /somedir/page.html HTTP/1.1\r\n\r\n");

    // change the number directly for now: strlen is not reliable
    ret = process_request(buf, 44, response);

    if(ret == ERR) {
        printf("Parsing failed.\n");
        return ERR;
    }

    printf("\nResponse:\n");
    printf("%s\n", response);

    return OK;
}
