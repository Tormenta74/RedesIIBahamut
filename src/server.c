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
#include "http.h"

struct server_options so;

int process_request(char *buf, size_t buflen, char *response) {
    char *date_buf;
    int ret;
    struct http_pairs res_headers[MAX_HEADERS];
    struct http_req_data rd;

    ret = http_request_parse(buf, buflen, &rd);

    http_request_data_print(&rd);

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

        ret = http_response_build(response, rd.version, 200, "OK", 2, 3, res_headers, NULL, 0);

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
